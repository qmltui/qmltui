#include "Canvas.h"
#include "ColorMapper.h"
#include <cmath>
#include <QJSValue>


namespace qtui {

static void drawBresenhamLine(Canvas* canvas, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x1, y = y1;
    while (true) {
        canvas->setPixelColorHex(x, y, color);
        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

// --- Canvas ---
Canvas::Canvas(QObject* parent) : Item(parent) {
    resizeBuffer();
}

void Canvas::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void Canvas::setFillColor(const QString& color) {
    if (m_fillColor == color) return;
    m_fillColor = color;
    m_resolvedFillColor = ColorMapper::fromString(color, 0x00000000);
    emit fillColorChanged();
    markDirty();
}

void Canvas::setRenderMode(RenderMode mode) {
    if (m_renderMode == mode) return;
    m_renderMode = mode;
    resizeBuffer();
    emit renderModeChanged();
    emit pixelWidthChanged();
    emit pixelHeightChanged();
    markDirty();
}

void Canvas::setAutoClear(bool autoClear) {
    if (m_autoClear == autoClear) return;
    m_autoClear = autoClear;
    emit autoClearChanged();
    markDirty();
}

void Canvas::setWidth(int width) {
    if (this->width() == width) return;
    Item::setWidth(width);
    resizeBuffer();
    emit pixelWidthChanged();
}

void Canvas::setHeight(int height) {
    if (this->height() == height) return;
    Item::setHeight(height);
    resizeBuffer();
    emit pixelHeightChanged();
}

void Canvas::setPixel(int x, int y, bool value) {
    if (m_renderMode == RenderMode::Braille) {
        int cellX = x / 2;
        int cellY = y / 4;
        if (cellX < 0 || cellX >= width() || cellY < 0 || cellY >= height()) return;
        int bitIndex = (y % 4) + (x % 2) * 4;
        int idx = cellY * width() + cellX;
        if (value) {
            m_cells[idx] |= (1 << bitIndex);
        } else {
            m_cells[idx] &= ~(1 << bitIndex);
        }
        markDirty();
    } else {
        uint32_t col = value ? m_resolvedColor : (m_fillColor.isEmpty() ? 0x00000000 : m_resolvedFillColor);
        setPixelColorHex(x, y, col);
    }
}

void Canvas::setPixelColor(int x, int y, const QString& color) {
    setPixelColorHex(x, y, ColorMapper::fromString(color, m_resolvedColor));
}

void Canvas::setPixelColorHex(int x, int y, uint32_t color) {
    int w = pixelWidth();
    int h = pixelHeight();
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    if (m_renderMode == RenderMode::Braille) {
        int idx = (y / 4) * width() + (x / 2);
        uint8_t old = m_cells[idx];
        m_cells[idx] |= (1 << ((y % 4) + (x % 2) * 4));
        if (m_cells[idx] != old) markDirty();
    } else {
        int idx = y * w + x;
        if (m_pixelColors[idx] != color) {
            m_pixelColors[idx] = color;
            markDirty();
        }
    }
}

void Canvas::clear() {
    uint32_t clearBg = m_fillColor.isEmpty() ? 0x00000000 : m_resolvedFillColor;
    std::fill(m_cells.begin(), m_cells.end(), 0x00);
    std::fill(m_pixelColors.begin(), m_pixelColors.end(), clearBg);
    markDirty();
}

void Canvas::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible()) return;

    resizeBuffer();
    if (m_autoClear) {
        if (m_renderMode == RenderMode::Braille) {
            std::fill(m_cells.begin(), m_cells.end(), 0x00);
        } else {
            uint32_t clearBg = m_fillColor.isEmpty() ? 0x00000000 : m_resolvedFillColor;
            std::fill(m_pixelColors.begin(), m_pixelColors.end(), clearBg);
        }
    }


    // Draw all visual children shapes
    for (Item* child : childItems()) {
        if (child && child->visible()) {
            if (CanvasShape* shape = qobject_cast<CanvasShape*>(child)) {
                shape->draw(this);
            }
        }
    }


    if (m_renderMode == RenderMode::Braille) {
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                uint8_t cellValue = m_cells[y * width() + x];
                if (cellValue > 0) {
                    int brailleChar = 0x2800 + cellValue;
                    QChar qc(brailleChar);
                    QString qs(qc);
                    QByteArray utf8 = qs.toUtf8();

                    Cell& cell = buffer.cellAt(absX + x, absY + y);
                    cell.character = utf8;
                    cell.displayWidth = 1;
                    cell.continuation = false;
                    cell.fgColor = m_resolvedColor;
                }
            }
        }
    } else { // HalfBlock
        int w = width();
        int h = height();
        for (int cellY = 0; cellY < h; ++cellY) {
            for (int cellX = 0; cellX < w; ++cellX) {
                int topY = cellY * 2;
                int bottomY = cellY * 2 + 1;

                uint32_t topColor = (topY < h * 2) ? m_pixelColors[topY * w + cellX] : 0x00000000;
                uint32_t bottomColor = (bottomY < h * 2) ? m_pixelColors[bottomY * w + cellX] : 0x00000000;

                bool topSet = (topColor >> 24) != 0;
                bool bottomSet = (bottomColor >> 24) != 0;

                if (!topSet && !bottomSet) {
                    continue;
                }

                Cell& cell = buffer.cellAt(absX + cellX, absY + cellY);
                cell.displayWidth = 1;
                cell.continuation = false;

                if (topSet && bottomSet) {
                    cell.character = "\xE2\x96\x80"; // ▀ Upper Half Block
                    cell.fgColor = topColor;
                    cell.bgColor = bottomColor;
                } else if (topSet) {
                    cell.character = "\xE2\x96\x80"; // ▀ Upper Half Block
                    cell.fgColor = topColor;
                    cell.bgColor = 0x00000000;
                } else {
                    cell.character = "\xE2\x96\x84"; // ▄ Lower Half Block
                    cell.fgColor = bottomColor;
                    cell.bgColor = 0x00000000;
                }
            }
        }
    }
}

void Canvas::resizeBuffer() {
    if (m_renderMode == RenderMode::Braille) {
        size_t needed = static_cast<size_t>(width() * height());
        if (m_cells.size() != needed) {
            m_cells.resize(needed, 0x00);
        }
    } else {
        size_t needed = static_cast<size_t>(width() * (height() * 2));
        if (m_pixelColors.size() != needed) {
            m_pixelColors.resize(needed, 0x00000000);
        }
    }
}

// --- CanvasShape ---
void CanvasShape::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(color, 0xFFFFFFFF);
    m_hasCustomColor = true;
    emit colorChanged();
    if (parentItem()) parentItem()->markDirty();
}

uint32_t CanvasShape::resolvedColor(Canvas* canvas) const {
    if (m_hasCustomColor) return m_resolvedColor;
    return canvas ? canvas->resolvedColor() : 0xFFFFFFFF;
}

// --- Point ---
Point::Point(QObject* parent) : CanvasShape(parent) {}

void Point::draw(Canvas* canvas) {
    canvas->setPixelColorHex(x(), y(), resolvedColor(canvas));
}

// --- Line ---
Line::Line(QObject* parent) : CanvasShape(parent) {}

void Line::setX1(int x1) {
    if (m_x1 == x1) return;
    m_x1 = x1;
    emit x1Changed();
    if (parentItem()) parentItem()->markDirty();
}

void Line::setY1(int y1) {
    if (m_y1 == y1) return;
    m_y1 = y1;
    emit y1Changed();
    if (parentItem()) parentItem()->markDirty();
}

void Line::setX2(int x2) {
    if (m_x2 == x2) return;
    m_x2 = x2;
    emit x2Changed();
    if (parentItem()) parentItem()->markDirty();
}

void Line::setY2(int y2) {
    if (m_y2 == y2) return;
    m_y2 = y2;
    emit y2Changed();
    if (parentItem()) parentItem()->markDirty();
}

void Line::draw(Canvas* canvas) {
    drawBresenhamLine(canvas, m_x1, m_y1, m_x2, m_y2, resolvedColor(canvas));
}

// --- Polygon ---
Polygon::Polygon(QObject* parent) : CanvasShape(parent) {}

void Polygon::setPoints(const QVariantList& points) {
    m_parsedPoints.clear();
    for (const QVariant& var : points) {
        int px = 0, py = 0;
        if (var.userType() == qMetaTypeId<QJSValue>()) {
            QJSValue jsVal = var.value<QJSValue>();
            px = jsVal.property("x").toInt();
            py = jsVal.property("y").toInt();
        } else if (var.canConvert<QVariantMap>()) {
            QVariantMap map = var.toMap();
            px = map.value("x").toInt();
            py = map.value("y").toInt();
        }
        m_parsedPoints.append(QPoint(px, py));
    }
    m_points = points;
    emit pointsChanged();
    if (parentItem()) parentItem()->markDirty();
}

void Polygon::setClosed(bool closed) {
    if (m_closed == closed) return;
    m_closed = closed;
    emit closedChanged();
    if (parentItem()) parentItem()->markDirty();
}

void Polygon::draw(Canvas* canvas) {
    if (m_parsedPoints.isEmpty()) return;

    uint32_t col = resolvedColor(canvas);
    for (int i = 0; i < m_parsedPoints.size() - 1; ++i) {
        drawBresenhamLine(canvas, m_parsedPoints[i].x(), m_parsedPoints[i].y(),
                         m_parsedPoints[i + 1].x(), m_parsedPoints[i + 1].y(), col);
    }

    if (m_closed && m_parsedPoints.size() > 2) {
        drawBresenhamLine(canvas, m_parsedPoints.back().x(), m_parsedPoints.back().y(),
                         m_parsedPoints.front().x(), m_parsedPoints.front().y(), col);
    }
}



// --- CanvasRectangle ---
CanvasRectangle::CanvasRectangle(QObject* parent) : CanvasShape(parent) {}

void CanvasRectangle::setFill(bool fill) { if (m_fill != fill) { m_fill = fill; emit fillChanged(); if (parentItem()) parentItem()->markDirty(); } }

void CanvasRectangle::draw(Canvas* canvas) {
    uint32_t col = resolvedColor(canvas);
    int rx = x();
    int ry = y();
    int rw = width();
    int rh = height();

    if (rw <= 0 || rh <= 0) return;

    if (m_fill) {
        for (int py = ry; py < ry + rh; ++py) {
            for (int px = rx; px < rx + rw; ++px) {
                canvas->setPixelColorHex(px, py, col);
            }
        }
    } else {
        drawBresenhamLine(canvas, rx, ry, rx + rw - 1, ry, col);
        drawBresenhamLine(canvas, rx, ry + rh - 1, rx + rw - 1, ry + rh - 1, col);
        drawBresenhamLine(canvas, rx, ry, rx, ry + rh - 1, col);
        drawBresenhamLine(canvas, rx + rw - 1, ry, rx + rw - 1, ry + rh - 1, col);
    }
}

// --- Circle ---
Circle::Circle(QObject* parent) : CanvasShape(parent) {}

void Circle::setCx(int cx) { m_cx = cx; m_cxSet = true; emit cxChanged(); if (parentItem()) parentItem()->markDirty(); }
void Circle::setCy(int cy) { m_cy = cy; m_cySet = true; emit cyChanged(); if (parentItem()) parentItem()->markDirty(); }
void Circle::setRadius(int r) { if (m_radius != r) { m_radius = r; emit radiusChanged(); if (parentItem()) parentItem()->markDirty(); } }
void Circle::setFill(bool fill) { if (m_fill != fill) { m_fill = fill; emit fillChanged(); if (parentItem()) parentItem()->markDirty(); } }

void Circle::draw(Canvas* canvas) {
    uint32_t col = resolvedColor(canvas);
    int r = m_radius;
    if (r <= 0) return;

    int centerx = cx();
    int centery = cy();
    int x = r;
    int y = 0;
    int err = 0;

    auto drawPoints = [&](int pcx, int pcy, int px, int py) {
        if (m_fill) {
            for (int ix = pcx - px; ix <= pcx + px; ++ix) {
                canvas->setPixelColorHex(ix, pcy + py, col);
                canvas->setPixelColorHex(ix, pcy - py, col);
            }
            for (int ix = pcx - py; ix <= pcx + py; ++ix) {
                canvas->setPixelColorHex(ix, pcy + px, col);
                canvas->setPixelColorHex(ix, pcy - px, col);
            }
        } else {
            canvas->setPixelColorHex(pcx + px, pcy + py, col);
            canvas->setPixelColorHex(pcx - px, pcy + py, col);
            canvas->setPixelColorHex(pcx + px, pcy - py, col);
            canvas->setPixelColorHex(pcx - px, pcy - py, col);
            canvas->setPixelColorHex(pcx + py, pcy + px, col);
            canvas->setPixelColorHex(pcx - py, pcy + px, col);
            canvas->setPixelColorHex(pcx + py, pcy - px, col);
            canvas->setPixelColorHex(pcx - py, pcy - px, col);
        }
    };

    while (x >= y) {
        drawPoints(centerx, centery, x, y);
        y += 1;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x -= 1;
            err += 1 - 2 * x;
        }
    }
}


} // namespace qtui
