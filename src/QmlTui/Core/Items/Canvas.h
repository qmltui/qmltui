#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QPoint>
#include <QVariant>
#include <QVariantList>
#include <vector>
#include <QmlTui/Core/Primitives/Subpixel.h>

namespace qmltui {

class CanvasPainter;
class CanvasShape;

// Subpixel drawing surface backed by Braille (2x4 dots/cell) or HalfBlock
// (1x2 blocks/cell) glyphs. Driven declaratively by child CanvasShape items
// and/or imperatively from QML via setPixel() and setPixelColor().
class QMLTUI_CORE_EXPORT Canvas : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qmltui::Color fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode NOTIFY renderModeChanged)
    Q_PROPERTY(CellColorMode cellColorMode READ cellColorMode WRITE setCellColorMode NOTIFY
                   cellColorModeChanged)
    // When true, the pixel buffer is cleared at the start of each paint() pass
    // before child CanvasShapes draw.
    Q_PROPERTY(bool autoClear READ autoClear WRITE setAutoClear NOTIFY autoClearChanged)
    Q_PROPERTY(int pixelWidth READ pixelWidth NOTIFY pixelWidthChanged)
    Q_PROPERTY(int pixelHeight READ pixelHeight NOTIFY pixelHeightChanged)

public:
    enum class RenderMode { Braille, HalfBlock };
    Q_ENUM(RenderMode)

    // How a Braille cell picks its color when dots of different colors share it.
    enum class CellColorMode {
        LastDot, // the most recently drawn dot decides (default, cheapest)
        Average  // mean of every colored dot drawn into the cell since clear()
    };
    Q_ENUM(CellColorMode)

    // Per-cell sums used by CellColorMode::Average.
    struct CellAccumulator {
        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;
        uint32_t count = 0;
    };

    explicit Canvas(QObject* parent = nullptr);
    ~Canvas() override = default;

    Color color() const { return m_color; }
    void setColor(Color color);
    uint32_t resolvedColor() const { return m_resolvedColor; }

    Color fillColor() const { return m_fillColor; }
    void setFillColor(Color color);
    uint32_t resolvedFillColor() const { return m_resolvedFillColor; }

    RenderMode renderMode() const { return m_renderMode; }
    void setRenderMode(RenderMode mode);

    CellColorMode cellColorMode() const { return m_cellColorMode; }

    void setCellColorMode(CellColorMode mode);

    bool autoClear() const { return m_autoClear; }
    void setAutoClear(bool autoClear);

    void setWidth(int width) override;
    void setHeight(int height) override;

    int pixelWidth() const {
        return m_renderMode == RenderMode::Braille ? width() * braille::DotsX
                                                   : width() * half_block::SubpixelsX;
    }
    int pixelHeight() const {
        return m_renderMode == RenderMode::Braille ? height() * braille::DotsY
                                                   : height() * half_block::SubpixelsY;
    }

    // Scripted drawing from QML. These persist until the next frame when
    // autoClear is true, or until clear() otherwise, and schedule a repaint.
    Q_INVOKABLE void setPixel(int x, int y, bool value = true);
    Q_INVOKABLE void setPixelColor(int x, int y, Color color);
    void setPixelColorHex(int x, int y, uint32_t color);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void requestPaint();

    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const override;

signals:
    void colorChanged();
    void fillColorChanged();
    void renderModeChanged();
    void cellColorModeChanged();
    void autoClearChanged();
    void pixelWidthChanged();
    void pixelHeightChanged();

private:
    friend class CanvasPainter;
    CanvasPainter painter() const;
    void resizeBuffer() const;

    RenderMode m_renderMode = RenderMode::Braille;
    bool m_autoClear = true;
    // Pixel storage is derived rendering state: shapes write it while the
    // canvas paints, so it is mutable and never marks the item dirty.
    mutable std::vector<uint8_t> m_cells; // Braille mode (2x4 bits per cell)
    // Braille mode ink per cell (ARGB). The last dot drawn into a cell decides its
    // color; 0 means "the canvas color at paint time", which is what setPixel() writes.
    mutable std::vector<uint32_t> m_cellColors;
    // Average mode accumulators, allocated only while that mode is active.
    mutable std::vector<CellAccumulator> m_cellAccumulators;
    CellColorMode m_cellColorMode = CellColorMode::LastDot;
    mutable std::vector<uint8_t> m_pixelSet;     // occupancy, independent of default color 0
    mutable std::vector<uint32_t> m_pixelColors; // HalfBlock mode (1x2 colors per cell, ARGB)
    Color m_color = "default";
    uint32_t m_resolvedColor = Color::resolve(m_color, 0xFFFFFFFF); // see Color.h
    Color m_fillColor;
    uint32_t m_resolvedFillColor = 0x00000000;
};

// The pixel surface shapes draw into during Canvas::paint(). It writes the
// canvas's pixel storage and nothing else, so painting stays const and never
// schedules another frame.
class CanvasPainter {
public:
    int pixelWidth() const { return m_canvas.pixelWidth(); }
    int pixelHeight() const { return m_canvas.pixelHeight(); }
    Canvas::RenderMode renderMode() const { return m_canvas.m_renderMode; }

    // Color used by shapes that do not set their own.
    uint32_t defaultColor() const { return m_canvas.m_resolvedColor; }

    void setPixel(int x, int y, bool value = true);
    void setPixelColor(int x, int y, Color color);
    void setPixelColorHex(int x, int y, uint32_t color);
    void clear();

private:
    friend class Canvas;
    explicit CanvasPainter(const Canvas& canvas) : m_canvas(canvas) {}
    const Canvas& m_canvas;
};

// Abstract base class for declarative subpixel shapes placed inside a Canvas.
// Coordinates on shapes are in Canvas subpixels (0..pixelWidth, 0..pixelHeight).
class QMLTUI_CORE_EXPORT CanvasShape : public Item {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("CanvasShape is an abstract base class")
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)

public:
    CanvasShape(QObject* parent = nullptr) : Item(parent) {}
    ~CanvasShape() override = default;

    Color color() const { return m_color; }
    void setColor(Color color);
    uint32_t resolvedColor(const CanvasPainter& painter) const;

    // Draws this shape into the parent canvas's pixel surface.
    virtual void draw(CanvasPainter& painter) const = 0;

    void paint(TextGridBuffer&, int, int, const ClipRect&) const override {
        // Shapes are drawn onto the parent Canvas buffer during Canvas::paint().
    }

signals:
    void colorChanged();

private:
    Color m_color;
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    bool m_hasCustomColor = false;
};

// Single subpixel dot at (x, y).
class QMLTUI_CORE_EXPORT Point : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT

public:
    Point(QObject* parent = nullptr);
    void draw(CanvasPainter& painter) const override;
};

// Bresenham line segment between subpixel coordinates (x1, y1) and (x2, y2).
class QMLTUI_CORE_EXPORT Line : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int x1 READ x1 WRITE setX1 NOTIFY x1Changed)
    Q_PROPERTY(int y1 READ y1 WRITE setY1 NOTIFY y1Changed)
    Q_PROPERTY(int x2 READ x2 WRITE setX2 NOTIFY x2Changed)
    Q_PROPERTY(int y2 READ y2 WRITE setY2 NOTIFY y2Changed)

public:
    Line(QObject* parent = nullptr);
    int x1() const { return m_x1; }
    void setX1(int x1);
    int y1() const { return m_y1; }
    void setY1(int y1);
    int x2() const { return m_x2; }
    void setX2(int x2);
    int y2() const { return m_y2; }
    void setY2(int y2);

signals:
    void x1Changed();
    void y1Changed();
    void x2Changed();
    void y2Changed();

public:
    void draw(CanvasPainter& painter) const override;

private:
    int m_x1 = 0;
    int m_y1 = 0;
    int m_x2 = 0;
    int m_y2 = 0;
};

// Polyline or closed polygon through a list of {x, y} or QPoint vertices.
class QMLTUI_CORE_EXPORT Polygon : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList points READ points WRITE setPoints NOTIFY pointsChanged)
    Q_PROPERTY(bool closed READ closed WRITE setClosed NOTIFY closedChanged)

public:
    Polygon(QObject* parent = nullptr);
    QVariantList points() const { return m_points; }
    void setPoints(const QVariantList& points);
    bool closed() const { return m_closed; }
    void setClosed(bool closed);

signals:
    void pointsChanged();
    void closedChanged();

public:
    void draw(CanvasPainter& painter) const override;

private:
    QVariantList m_points;
    QList<QPoint> m_parsedPoints;
    bool m_closed = true;
};

// Stroked or filled subpixel rectangle at (x, y) with size (width, height).
class QMLTUI_CORE_EXPORT CanvasRectangle : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)

public:
    CanvasRectangle(QObject* parent = nullptr);
    bool fill() const { return m_fill; }
    void setFill(bool fill);

signals:
    void fillChanged();

public:
    void draw(CanvasPainter& painter) const override;

private:
    bool m_fill = false;
};

// Stroked or filled subpixel circle centered at (cx, cy) (falling back to x, y
// when cx/cy are unset) with the given radius.
class QMLTUI_CORE_EXPORT Circle : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int cx READ cx WRITE setCx NOTIFY cxChanged)
    Q_PROPERTY(int cy READ cy WRITE setCy NOTIFY cyChanged)
    Q_PROPERTY(int radius READ radius WRITE setRadius NOTIFY radiusChanged)
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)

public:
    Circle(QObject* parent = nullptr);
    int cx() const { return m_cxSet ? m_cx : x(); }
    void setCx(int cx);
    int cy() const { return m_cySet ? m_cy : y(); }
    void setCy(int cy);
    int radius() const { return m_radius; }
    void setRadius(int r);
    bool fill() const { return m_fill; }
    void setFill(bool fill);

signals:
    void cxChanged();
    void cyChanged();
    void radiusChanged();
    void fillChanged();

public:
    void draw(CanvasPainter& painter) const override;

private:
    int m_cx = 0;
    int m_cy = 0;
    bool m_cxSet = false;
    bool m_cySet = false;
    int m_radius = 0;
    bool m_fill = false;
};

} // namespace qmltui
