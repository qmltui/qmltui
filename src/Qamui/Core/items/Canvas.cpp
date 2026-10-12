#include <Qamui/Core/items/Canvas.h>
#include <Qamui/Core/primitives/ColorMapper.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <QJSValue>


namespace qamui {

namespace {

void drawBresenhamLine(CanvasPainter& painter, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x1, y = y1;
    while (true) {
        painter.setPixelColorHex(x, y, color);
        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

} // namespace

// Average mode: fold one colored dot into its cell. Colors without RGB (terminal
// default or transparent) carry no information and are skipped.
static void accumulateCellColor(std::vector<Canvas::CellAccumulator>& accumulators, size_t idx, uint32_t color) {
    if (idx >= accumulators.size()) return;
    const uint32_t rgb = ColorMapper::toArgb(color);
    if ((rgb >> 24) == 0) return;
    Canvas::CellAccumulator& acc = accumulators[idx];
    acc.red += (rgb >> 16) & 0xFF;
    acc.green += (rgb >> 8) & 0xFF;
    acc.blue += rgb & 0xFF;
    ++acc.count;
}

// --- CanvasPainter ---
void CanvasPainter::setPixel(int x, int y, bool value) {
    if (m_canvas.m_renderMode == Canvas::RenderMode::Braille) {
        const int cellX = x / braille::DotsX;
        const int cellY = y / braille::DotsY;
        if (x < 0 || y < 0 || cellX >= m_canvas.width() || cellY >= m_canvas.height()) return;
        const uint8_t bitMask = braille::dotMask(x % braille::DotsX, y % braille::DotsY);
        const size_t idx = static_cast<size_t>(cellY * m_canvas.width() + cellX);
        if (idx >= m_canvas.m_cells.size()) return;
        if (value) {
            m_canvas.m_cells[idx] |= bitMask;
            if (idx < m_canvas.m_cellColors.size())
                m_canvas.m_cellColors[idx] = 0x00000000;   // follow the canvas color
            if (m_canvas.m_cellColorMode == Canvas::CellColorMode::Average)
                accumulateCellColor(m_canvas.m_cellAccumulators, idx, m_canvas.m_resolvedColor);
        } else {
            m_canvas.m_cells[idx] &= static_cast<uint8_t>(~bitMask);
        }
    } else {
        const uint32_t color = value
            ? m_canvas.m_resolvedColor
            : (!m_canvas.m_fillColor.isValid() ? 0x00000000 : m_canvas.m_resolvedFillColor);
        setPixelColorHex(x, y, color);
        if (!value && (m_canvas.m_resolvedFillColor >> 24) == 0
            && x >= 0 && y >= 0 && x < pixelWidth() && y < pixelHeight()) {
            m_canvas.m_pixelSet[static_cast<size_t>(y * pixelWidth() + x)] = false;
        }
    }
}

void CanvasPainter::setPixelColor(int x, int y, Color color) {
    setPixelColorHex(x, y, ColorMapper::toPaintedColor(
        ColorMapper::fromString(color, m_canvas.m_resolvedColor)));
}

void CanvasPainter::setPixelColorHex(int x, int y, uint32_t color) {
    const int w = pixelWidth();
    const int h = pixelHeight();
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    if (m_canvas.m_renderMode == Canvas::RenderMode::Braille) {
        const size_t idx = static_cast<size_t>((y / braille::DotsY) * m_canvas.width() + (x / braille::DotsX));
        if (idx < m_canvas.m_cells.size()) {
            m_canvas.m_cells[idx] |= braille::dotMask(x % braille::DotsX, y % braille::DotsY);
            if (idx < m_canvas.m_cellColors.size())
                m_canvas.m_cellColors[idx] = color;         // last dot drawn wins the cell
            if (m_canvas.m_cellColorMode == Canvas::CellColorMode::Average)
                accumulateCellColor(m_canvas.m_cellAccumulators, idx, color);
        }
    } else {
        const size_t idx = static_cast<size_t>(y * w + x);
        if (idx < m_canvas.m_pixelColors.size()) {
            m_canvas.m_pixelColors[idx] = color;
            m_canvas.m_pixelSet[idx] = true;
        }
    }
}

void CanvasPainter::clear() {
    const uint32_t clearBg = !m_canvas.m_fillColor.isValid() ? 0x00000000 : m_canvas.m_resolvedFillColor;
    std::fill(m_canvas.m_cells.begin(), m_canvas.m_cells.end(), 0x00);
    std::fill(m_canvas.m_cellColors.begin(), m_canvas.m_cellColors.end(), 0x00000000);
    std::fill(m_canvas.m_cellAccumulators.begin(), m_canvas.m_cellAccumulators.end(), Canvas::CellAccumulator{});
    std::fill(m_canvas.m_pixelColors.begin(), m_canvas.m_pixelColors.end(), clearBg);
    // A transparent fill clears to holes, while a drawn default-color pixel
    // is occupied and uses the terminal's foreground.
    std::fill(m_canvas.m_pixelSet.begin(), m_canvas.m_pixelSet.end(), (clearBg >> 24) != 0);
}

// --- Canvas ---
Canvas::Canvas(QObject* parent) : Item(parent) {
    resizeBuffer();
}

CanvasPainter Canvas::painter() const {
    return CanvasPainter(*this);
}

void Canvas::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::toPaintedColor(
        ColorMapper::fromString(color, 0xFFFFFFFF));
    emit colorChanged();
    markDirty();
}

void Canvas::setFillColor(Color color) {
    if (m_fillColor == color) return;
    m_fillColor = color;
    // A transparent fill means no ink, which the canvas already spells 0.
    m_resolvedFillColor = ColorMapper::toPaintedColor(
        ColorMapper::fromString(color, 0x00000000));
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

void Canvas::setCellColorMode(CellColorMode mode) {
    if (m_cellColorMode == mode) return;
    m_cellColorMode = mode;
    if (mode == CellColorMode::Average) {
        resizeBuffer();
    } else {
        m_cellAccumulators.clear();
        m_cellAccumulators.shrink_to_fit();
    }
    emit cellColorModeChanged();
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
    resizeBuffer();
    painter().setPixel(x, y, value);
    markDirty();
}

void Canvas::setPixelColor(int x, int y, Color color) {
    resizeBuffer();
    painter().setPixelColor(x, y, color);
    markDirty();
}

void Canvas::setPixelColorHex(int x, int y, uint32_t color) {
    resizeBuffer();
    painter().setPixelColorHex(x, y, color);
    markDirty();
}

void Canvas::clear() {
    painter().clear();
    markDirty();
}

void Canvas::requestPaint() {
    markDirty();
}

void Canvas::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible()) return;

    resizeBuffer();
    CanvasPainter surface = painter();
    if (m_autoClear)
        surface.clear();

    // Draw all visual children shapes into the pixel surface.
    for (Item* child : childItems()) {
        if (child && child->visible()) {
            if (const CanvasShape* shape = qobject_cast<const CanvasShape*>(child)) {
                shape->draw(surface);
            }
        }
    }

    if (m_renderMode == RenderMode::Braille) {
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                if (!clipRect.contains(absX + x, absY + y))
                    continue;
                const uint8_t cellValue = m_cells[static_cast<size_t>(y * width() + x)];
                if (cellValue == 0)
                    continue;
                Cell *targetCell = buffer.tryCellAt(absX + x, absY + y);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                cell.setCharacter(braille::utf8(cellValue));
                cell.displayWidth = 1;
                cell.continuation = false;
                const size_t cellIdx = static_cast<size_t>(y * width() + x);
                const uint32_t ink = m_cellColors[cellIdx];
                cell.fgColor = ink ? ink : m_resolvedColor;
                if (m_cellColorMode == CellColorMode::Average && cellIdx < m_cellAccumulators.size()) {
                    const CellAccumulator& acc = m_cellAccumulators[cellIdx];
                    if (acc.count > 0) {
                        cell.fgColor = 0xFF000000u
                            | ((acc.red / acc.count) << 16)
                            | ((acc.green / acc.count) << 8)
                            | (acc.blue / acc.count);
                    }
                }
            }
        }
    } else { // HalfBlock
        const int w = width();
        const int h = height();
        for (int cellY = 0; cellY < h; ++cellY) {
            for (int cellX = 0; cellX < w; ++cellX) {
                if (!clipRect.contains(absX + cellX, absY + cellY))
                    continue;
                const int topY = cellY * half_block::SubpixelsY;
                const int bottomY = cellY * half_block::SubpixelsY + 1;

                const uint32_t topColor = (topY < h * half_block::SubpixelsY) ? m_pixelColors[static_cast<size_t>(topY * w + cellX)] : 0x00000000;
                const uint32_t bottomColor = (bottomY < h * half_block::SubpixelsY) ? m_pixelColors[static_cast<size_t>(bottomY * w + cellX)] : 0x00000000;

                const bool topSet = m_pixelSet[static_cast<size_t>(topY * w + cellX)];
                const bool bottomSet = m_pixelSet[static_cast<size_t>(bottomY * w + cellX)];
                if (!topSet && !bottomSet)
                    continue;

                Cell *targetCell = buffer.tryCellAt(absX + cellX, absY + cellY);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                cell.displayWidth = 1;
                cell.continuation = false;

                if (topSet && bottomSet && (topColor >> 24) == 0 && (bottomColor >> 24) == 0) {
                    // SGR 49 is the default background, not the default ink.
                    // A full block paints both pixels with the foreground.
                    cell.setCharacter(block_glyphs::Full);
                    cell.fgColor = 0;
                    cell.bgColor = 0;
                } else if (topSet && bottomSet && (bottomColor >> 24) == 0) {
                    cell.setCharacter(block_glyphs::LowerHalf);
                    cell.fgColor = bottomColor;
                    cell.bgColor = topColor;
                } else if (topSet && bottomSet) {
                    cell.setCharacter(block_glyphs::UpperHalf);
                    cell.fgColor = topColor;
                    cell.bgColor = bottomColor;
                } else if (topSet) {
                    cell.setCharacter(block_glyphs::UpperHalf);
                    cell.fgColor = topColor;
                    cell.bgColor = 0x00000000;
                } else {
                    cell.setCharacter(block_glyphs::LowerHalf);
                    cell.fgColor = bottomColor;
                    cell.bgColor = 0x00000000;
                }
            }
        }
    }
}

void Canvas::resizeBuffer() const {
    if (m_renderMode == RenderMode::Braille) {
        const size_t needed = static_cast<size_t>(std::max(0, width()) * std::max(0, height()));
        if (m_cells.size() != needed || m_cellColors.size() != needed) {
            m_cells.resize(needed, 0x00);
            m_cellColors.resize(needed, 0x00000000);
        }
        const size_t accumulatorsNeeded = m_cellColorMode == CellColorMode::Average ? needed : 0;
        if (m_cellAccumulators.size() != accumulatorsNeeded)
            m_cellAccumulators.assign(accumulatorsNeeded, CellAccumulator{});
    } else {
        const size_t needed = static_cast<size_t>(std::max(0, width()) * (std::max(0, height()) * half_block::SubpixelsY));
        if (m_pixelColors.size() != needed) {
            m_pixelColors.resize(needed, 0x00000000);
            m_pixelSet.resize(needed, false);
        }
    }
}

// --- CanvasShape ---
void CanvasShape::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::toPaintedColor(
        ColorMapper::fromString(color, 0xFFFFFFFF));
    m_hasCustomColor = true;
    emit colorChanged();
    if (parentItem()) parentItem()->markDirty();
}

uint32_t CanvasShape::resolvedColor(const CanvasPainter& painter) const {
    if (m_hasCustomColor) return m_resolvedColor;
    return painter.defaultColor();
}

// --- Point ---
Point::Point(QObject* parent) : CanvasShape(parent) {}

void Point::draw(CanvasPainter& painter) const {
    painter.setPixelColorHex(x(), y(), resolvedColor(painter));
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

void Line::draw(CanvasPainter& painter) const {
    drawBresenhamLine(painter, m_x1, m_y1, m_x2, m_y2, resolvedColor(painter));
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

void Polygon::draw(CanvasPainter& painter) const {
    if (m_parsedPoints.isEmpty()) return;

    const uint32_t col = resolvedColor(painter);
    for (int i = 0; i < m_parsedPoints.size() - 1; ++i) {
        drawBresenhamLine(painter, m_parsedPoints[i].x(), m_parsedPoints[i].y(),
                          m_parsedPoints[i + 1].x(), m_parsedPoints[i + 1].y(), col);
    }

    if (m_closed && m_parsedPoints.size() > 2) {
        drawBresenhamLine(painter, m_parsedPoints.back().x(), m_parsedPoints.back().y(),
                          m_parsedPoints.front().x(), m_parsedPoints.front().y(), col);
    }
}



// --- CanvasRectangle ---
CanvasRectangle::CanvasRectangle(QObject* parent) : CanvasShape(parent) {}

void CanvasRectangle::setFill(bool fill) { if (m_fill != fill) { m_fill = fill; emit fillChanged(); if (parentItem()) parentItem()->markDirty(); } }

void CanvasRectangle::draw(CanvasPainter& painter) const {
    const uint32_t col = resolvedColor(painter);
    const int rx = x();
    const int ry = y();
    const int rw = width();
    const int rh = height();

    if (rw <= 0 || rh <= 0) return;

    if (m_fill) {
        for (int py = ry; py < ry + rh; ++py) {
            for (int px = rx; px < rx + rw; ++px) {
                painter.setPixelColorHex(px, py, col);
            }
        }
    } else {
        drawBresenhamLine(painter, rx, ry, rx + rw - 1, ry, col);
        drawBresenhamLine(painter, rx, ry + rh - 1, rx + rw - 1, ry + rh - 1, col);
        drawBresenhamLine(painter, rx, ry, rx, ry + rh - 1, col);
        drawBresenhamLine(painter, rx + rw - 1, ry, rx + rw - 1, ry + rh - 1, col);
    }
}

// --- Circle ---
Circle::Circle(QObject* parent) : CanvasShape(parent) {}

void Circle::setCx(int cx) { m_cx = cx; m_cxSet = true; emit cxChanged(); if (parentItem()) parentItem()->markDirty(); }
void Circle::setCy(int cy) { m_cy = cy; m_cySet = true; emit cyChanged(); if (parentItem()) parentItem()->markDirty(); }
void Circle::setRadius(int r) { if (m_radius != r) { m_radius = r; emit radiusChanged(); if (parentItem()) parentItem()->markDirty(); } }
void Circle::setFill(bool fill) { if (m_fill != fill) { m_fill = fill; emit fillChanged(); if (parentItem()) parentItem()->markDirty(); } }

void Circle::draw(CanvasPainter& painter) const {
    const uint32_t col = resolvedColor(painter);
    const int r = m_radius;
    if (r <= 0) return;

    const int centerx = cx();
    const int centery = cy();
    int x = r;
    int y = 0;
    // Midpoint decision between the next vertical and diagonal pixel steps.
    int err = 1 - r;

    auto drawPoints = [&](int pcx, int pcy, int px, int py) {
        if (m_fill) {
            for (int ix = pcx - px; ix <= pcx + px; ++ix) {
                painter.setPixelColorHex(ix, pcy + py, col);
                painter.setPixelColorHex(ix, pcy - py, col);
            }
            for (int ix = pcx - py; ix <= pcx + py; ++ix) {
                painter.setPixelColorHex(ix, pcy + px, col);
                painter.setPixelColorHex(ix, pcy - px, col);
            }
        } else {
            painter.setPixelColorHex(pcx + px, pcy + py, col);
            painter.setPixelColorHex(pcx - px, pcy + py, col);
            painter.setPixelColorHex(pcx + px, pcy - py, col);
            painter.setPixelColorHex(pcx - px, pcy - py, col);
            painter.setPixelColorHex(pcx + py, pcy + px, col);
            painter.setPixelColorHex(pcx - py, pcy + px, col);
            painter.setPixelColorHex(pcx + py, pcy - px, col);
            painter.setPixelColorHex(pcx - py, pcy - px, col);
        }
    };

    while (x >= y) {
        drawPoints(centerx, centery, x, y);
        y += 1;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x -= 1;
            err += 2 * (y - x) + 1;
        }
    }
}


} // namespace qamui
