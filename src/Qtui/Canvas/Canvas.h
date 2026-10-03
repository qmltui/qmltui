#pragma once

#include "Item.h"
#include <QVariant>
#include <QVariantList>
#include <vector>

namespace qtui {

class CanvasShape;

class Canvas : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode NOTIFY renderModeChanged)
    Q_PROPERTY(bool autoClear READ autoClear WRITE setAutoClear NOTIFY autoClearChanged)
    Q_PROPERTY(int pixelWidth READ pixelWidth NOTIFY pixelWidthChanged)
    Q_PROPERTY(int pixelHeight READ pixelHeight NOTIFY pixelHeightChanged)

public:
    enum class RenderMode {
        Braille,
        HalfBlock
    };
    Q_ENUM(RenderMode)

    Canvas(QObject* parent = nullptr);
    ~Canvas() override = default;

    QString color() const { return m_color; }
    void setColor(const QString& color);
    uint32_t resolvedColor() const { return m_resolvedColor; }

    QString fillColor() const { return m_fillColor; }
    void setFillColor(const QString& color);
    uint32_t resolvedFillColor() const { return m_resolvedFillColor; }

    RenderMode renderMode() const { return m_renderMode; }
    void setRenderMode(RenderMode mode);

    bool autoClear() const { return m_autoClear; }
    void setAutoClear(bool autoClear);

    void setWidth(int width) override;
    void setHeight(int height) override;

    int pixelWidth() const { return m_renderMode == RenderMode::Braille ? width() * 2 : width(); }
    int pixelHeight() const { return m_renderMode == RenderMode::Braille ? height() * 4 : height() * 2; }

    Q_INVOKABLE void setPixel(int x, int y, bool value = true);
    Q_INVOKABLE void setPixelColor(int x, int y, const QString& color);
    void setPixelColorHex(int x, int y, uint32_t color);

    Q_INVOKABLE void clear();

    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) override;

signals:
    void colorChanged();
    void fillColorChanged();
    void renderModeChanged();
    void autoClearChanged();
    void pixelWidthChanged();
    void pixelHeightChanged();

private:
    void resizeBuffer();
    RenderMode m_renderMode = RenderMode::Braille;
    bool m_autoClear = true;
    std::vector<uint8_t> m_cells;            // For Braille mode (2x4 bits per cell)
    std::vector<uint32_t> m_pixelColors;     // For HalfBlock mode (1x2 colors per cell, ARGB)
    QString m_color = "white";
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    QString m_fillColor;
    uint32_t m_resolvedFillColor = 0x00000000;
};

class CanvasShape : public Item {
    Q_OBJECT
    QML_UNCREATABLE("CanvasShape is an abstract base class")
    Q_PROPERTY(QString color READ color WRITE setColor NOTIFY colorChanged)

public:
    CanvasShape(QObject* parent = nullptr) : Item(parent) {}
    ~CanvasShape() override = default;

    QString color() const { return m_color; }
    void setColor(const QString& color);
    uint32_t resolvedColor(Canvas* canvas) const;

    virtual void draw(Canvas* canvas) = 0;

    void render(TextGridBuffer&, int, int, const ClipRect&) override {
        // Shapes are drawn onto parent Canvas buffer during Canvas::render()
    }

signals:
    void colorChanged();

private:
    QString m_color;
    uint32_t m_resolvedColor = 0xFFFFFFFF;
    bool m_hasCustomColor = false;
};

class Point : public CanvasShape {
    Q_OBJECT
    QML_ELEMENT

public:
    Point(QObject* parent = nullptr);
    void draw(Canvas* canvas) override;
};

class Line : public CanvasShape {
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
    void draw(Canvas* canvas) override;

private:
    int m_x1 = 0;
    int m_y1 = 0;
    int m_x2 = 0;
    int m_y2 = 0;
};

#include <QPoint>

class Polygon : public CanvasShape {
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
    void draw(Canvas* canvas) override;

private:
    QVariantList m_points;
    QList<QPoint> m_parsedPoints;
    bool m_closed = true;
};


class CanvasRectangle : public CanvasShape {
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
    void draw(Canvas* canvas) override;

private:
    bool m_fill = false;
};

class Circle : public CanvasShape {
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
    void draw(Canvas* canvas) override;

private:
    int m_cx = 0;
    int m_cy = 0;
    bool m_cxSet = false;
    bool m_cySet = false;
    int m_radius = 0;
    bool m_fill = false;
};

} // namespace qtui

