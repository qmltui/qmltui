#include <QmlTui/Core/Items/Canvas.h>
#include <algorithm>
#include <QJSValue>

namespace qmltui {

namespace {

void drawBresenhamLine(CanvasPainter& painter, const CanvasShape& shape, int x1, int y1, int x2,
                       int y2, int minX, int minY, int maxX, int maxY) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x1, y = y1;
    while (true) {
        painter.setPixelColorHex(x, y,
                                 shape.resolvePixelColor(painter, x, y, minX, minY, maxX, maxY));
        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
}

} // namespace

// Average mode: fold one colored dot into its cell. Colors without RGB (terminal
// default or transparent) carry no information and are skipped.
static void accumulateCellColor(std::vector<Canvas::CellAccumulator>& accumulators, size_t idx,
                                uint32_t color) {
    if (idx >= accumulators.size()) return;
    const uint32_t rgb = Color::toArgb(color);
    if ((rgb >> 24) == 0) return;
    Canvas::CellAccumulator& acc = accumulators[idx];
    acc.red += (rgb >> 16) & 0xFF;
    acc.green += (rgb >> 8) & 0xFF;
    acc.blue += rgb & 0xFF;
    ++acc.count;
}

void CanvasPainter::applyCellAttributes(int cellX, int cellY) {
    if (cellX < 0 || cellY < 0 || cellX >= m_canvas.width() || cellY >= m_canvas.height()) return;
    const size_t idx = static_cast<size_t>(cellY * m_canvas.width() + cellX);
    if (idx >= m_canvas.m_cellAttributes.size()) return;
    if (m_hasActiveAttributes) {
        m_canvas.m_cellAttributes[idx] = m_activeAttributes;
        m_canvas.m_cellHasCustomAttributes[idx] = 1;
    } else {
        m_canvas.m_cellHasCustomAttributes[idx] = 0;
    }
}

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
            const uint32_t defaultCol = m_canvas.defaultColorAt(x, y);
            if (idx < m_canvas.m_cellColors.size())
                m_canvas.m_cellColors[idx] = m_canvas.hasGradient() ? defaultCol : 0x00000000;
            if (m_canvas.m_cellColorMode == Canvas::CellColorMode::Average)
                accumulateCellColor(m_canvas.m_cellAccumulators, idx, defaultCol);
            applyCellAttributes(cellX, cellY);
        } else {
            m_canvas.m_cells[idx] &= static_cast<uint8_t>(~bitMask);
        }
    } else {
        const uint32_t color =
            value ? m_canvas.defaultColorAt(x, y)
                  : (!m_canvas.m_fillColor.isValid() ? 0x00000000 : m_canvas.m_resolvedFillColor);
        setPixelColorHex(x, y, color);
        if (!value && (m_canvas.m_resolvedFillColor >> 24) == 0 && x >= 0 && y >= 0 &&
            x < pixelWidth() && y < pixelHeight()) {
            m_canvas.m_pixelSet[static_cast<size_t>(y * pixelWidth() + x)] = false;
        }
    }
}

void CanvasPainter::setPixelColor(int x, int y, Color color) {
    setPixelColorHex(x, y, Color::paintedArgb32(Color::resolve(color, m_canvas.m_resolvedColor)));
}

void CanvasPainter::setPixelColorHex(int x, int y, uint32_t color) {
    const int w = pixelWidth();
    const int h = pixelHeight();
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    if (m_canvas.m_renderMode == Canvas::RenderMode::Braille) {
        const int cellX = x / braille::DotsX;
        const int cellY = y / braille::DotsY;
        const size_t idx = static_cast<size_t>(cellY * m_canvas.width() + cellX);
        if (idx < m_canvas.m_cells.size()) {
            m_canvas.m_cells[idx] |= braille::dotMask(x % braille::DotsX, y % braille::DotsY);
            if (idx < m_canvas.m_cellColors.size())
                m_canvas.m_cellColors[idx] = color; // last dot drawn wins the cell
            if (m_canvas.m_cellColorMode == Canvas::CellColorMode::Average)
                accumulateCellColor(m_canvas.m_cellAccumulators, idx, color);
            applyCellAttributes(cellX, cellY);
        }
    } else {
        const size_t idx = static_cast<size_t>(y * w + x);
        if (idx < m_canvas.m_pixelColors.size()) {
            m_canvas.m_pixelColors[idx] = color;
            m_canvas.m_pixelSet[idx] = true;
            applyCellAttributes(x / half_block::SubpixelsX, y / half_block::SubpixelsY);
        }
    }
}

void CanvasPainter::clear() {
    const uint32_t clearBg =
        !m_canvas.m_fillColor.isValid() ? 0x00000000 : m_canvas.m_resolvedFillColor;
    std::fill(m_canvas.m_cells.begin(), m_canvas.m_cells.end(), 0x00);
    std::fill(m_canvas.m_cellColors.begin(), m_canvas.m_cellColors.end(), 0x00000000);
    std::fill(m_canvas.m_cellAccumulators.begin(), m_canvas.m_cellAccumulators.end(),
              Canvas::CellAccumulator{});
    std::fill(m_canvas.m_pixelColors.begin(), m_canvas.m_pixelColors.end(), clearBg);
    // A transparent fill clears to holes, while a drawn default-color pixel
    // is occupied and uses the terminal's foreground.
    std::fill(m_canvas.m_pixelSet.begin(), m_canvas.m_pixelSet.end(), (clearBg >> 24) != 0);
    std::fill(m_canvas.m_cellAttributes.begin(), m_canvas.m_cellAttributes.end(),
              TextAttribute::None);
    std::fill(m_canvas.m_cellHasCustomAttributes.begin(), m_canvas.m_cellHasCustomAttributes.end(),
              0);
}

Canvas::Canvas(QObject* parent) : Item(parent) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    resizeBuffer();
}

CanvasPainter Canvas::painter() const {
    return CanvasPainter(*this);
}

void Canvas::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::paintedArgb32(Color::resolve(color, Color::DefaultTag));
    emit colorChanged();
    markDirty();
}

void Canvas::setGradient(Gradient* gradient) {
    if (m_gradient.data() == gradient) return;
    if (m_gradient) {
        disconnect(m_gradient.data(), nullptr, this, nullptr);
    }
    m_gradient = gradient;
    if (m_gradient) {
        connect(m_gradient.data(), &Gradient::updated, this, [this]() { markDirty(); });
        connect(m_gradient.data(), &QObject::destroyed, this, [this]() {
            m_gradient = nullptr;
            emit gradientChanged();
            markDirty();
        });
    }
    emit gradientChanged();
    markDirty();
}

uint32_t Canvas::defaultColorAt(int x, int y) const {
    if (!hasGradient()) return m_resolvedColor;
    qreal t = 0.0;
    if (m_gradient->orientation() == Gradient::Horizontal) {
        const int pw = pixelWidth();
        t = (pw > 1) ? static_cast<qreal>(x) / (pw - 1) : 0.0;
    } else {
        const int ph = pixelHeight();
        t = (ph > 1) ? static_cast<qreal>(y) / (ph - 1) : 0.0;
    }
    return Color::paintedArgb32(m_gradient->colorArgbAt(t));
}

void Canvas::setFillColor(Color color) {
    if (m_fillColor == color) return;
    m_fillColor = color;
    // A transparent fill means no ink, which the canvas already spells 0.
    m_resolvedFillColor = Color::paintedArgb32(Color::resolve(color, 0x00000000));
    emit fillColorChanged();
    markDirty();
}

void Canvas::setDim(bool dim) {
    if (m_dim == dim) return;
    m_dim = dim;
    emit dimChanged();
    markDirty();
}

TextAttribute Canvas::baseAttributes() const {
    return m_dim ? TextAttribute::Dim : TextAttribute::None;
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
    if (m_autoClear) surface.clear();

    // Draw all visual children shapes into the pixel surface.
    for (Item* child : childItems()) {
        if (child && child->visible()) {
            if (const CanvasShape* shape = qobject_cast<const CanvasShape*>(child)) {
                surface.setActiveAttributes(shape->resolvedAttributes(surface),
                                            shape->hasCustomAttributes());
                shape->draw(surface);
            }
        }
    }
    surface.setActiveAttributes(TextAttribute::None, false);

    const TextAttribute baseAttr = baseAttributes();

    if (m_renderMode == RenderMode::Braille) {
        for (int y = 0; y < height(); ++y) {
            for (int x = 0; x < width(); ++x) {
                if (!clipRect.contains(absX + x, absY + y)) continue;
                const uint8_t cellValue = m_cells[static_cast<size_t>(y * width() + x)];
                if (cellValue == 0) continue;
                Cell* targetCell = buffer.tryCellAt(absX + x, absY + y);
                if (!targetCell) continue;
                Cell& cell = *targetCell;
                cell.setCharacter(braille::utf8(cellValue));
                cell.displayWidth = 1;
                cell.continuation = false;
                const size_t cellIdx = static_cast<size_t>(y * width() + x);
                cell.attributes =
                    m_cellHasCustomAttributes[cellIdx] ? m_cellAttributes[cellIdx] : baseAttr;
                const uint32_t ink = m_cellColors[cellIdx];
                cell.fgColor = ink ? ink : defaultColorAt(x * braille::DotsX, y * braille::DotsY);
                if (m_cellColorMode == CellColorMode::Average &&
                    cellIdx < m_cellAccumulators.size()) {
                    const CellAccumulator& acc = m_cellAccumulators[cellIdx];
                    if (acc.count > 0) {
                        cell.fgColor = 0xFF000000u | ((acc.red / acc.count) << 16) |
                                       ((acc.green / acc.count) << 8) | (acc.blue / acc.count);
                    }
                }
            }
        }
    } else { // HalfBlock
        const int w = width();
        const int h = height();
        for (int cellY = 0; cellY < h; ++cellY) {
            for (int cellX = 0; cellX < w; ++cellX) {
                if (!clipRect.contains(absX + cellX, absY + cellY)) continue;
                const int topY = cellY * half_block::SubpixelsY;
                const int bottomY = cellY * half_block::SubpixelsY + 1;

                const uint32_t topColor = (topY < h * half_block::SubpixelsY)
                                              ? m_pixelColors[static_cast<size_t>(topY * w + cellX)]
                                              : 0x00000000;
                const uint32_t bottomColor =
                    (bottomY < h * half_block::SubpixelsY)
                        ? m_pixelColors[static_cast<size_t>(bottomY * w + cellX)]
                        : 0x00000000;

                const bool topSet = m_pixelSet[static_cast<size_t>(topY * w + cellX)];
                const bool bottomSet = m_pixelSet[static_cast<size_t>(bottomY * w + cellX)];
                if (!topSet && !bottomSet) continue;

                Cell* targetCell = buffer.tryCellAt(absX + cellX, absY + cellY);
                if (!targetCell) continue;
                Cell& cell = *targetCell;
                cell.displayWidth = 1;
                cell.continuation = false;
                const size_t cellIdx = static_cast<size_t>(cellY * w + cellX);
                cell.attributes =
                    m_cellHasCustomAttributes[cellIdx] ? m_cellAttributes[cellIdx] : baseAttr;

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
    const size_t cellCount = static_cast<size_t>((std::max)(0, width()) * (std::max)(0, height()));
    if (m_cellAttributes.size() != cellCount || m_cellHasCustomAttributes.size() != cellCount) {
        m_cellAttributes.resize(cellCount, TextAttribute::None);
        m_cellHasCustomAttributes.resize(cellCount, 0);
    }
    if (m_renderMode == RenderMode::Braille) {
        if (m_cells.size() != cellCount || m_cellColors.size() != cellCount) {
            m_cells.resize(cellCount, 0x00);
            m_cellColors.resize(cellCount, 0x00000000);
        }
        const size_t accumulatorsNeeded = m_cellColorMode == CellColorMode::Average ? cellCount : 0;
        if (m_cellAccumulators.size() != accumulatorsNeeded)
            m_cellAccumulators.assign(accumulatorsNeeded, CellAccumulator{});
    } else {
        const size_t needed = static_cast<size_t>(
            (std::max)(0, width()) * ((std::max)(0, height()) * half_block::SubpixelsY));
        if (m_pixelColors.size() != needed) {
            m_pixelColors.resize(needed, 0x00000000);
            m_pixelSet.resize(needed, false);
        }
    }
}

void CanvasShape::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::paintedArgb32(Color::resolve(color, Color::DefaultTag));
    m_hasCustomColor = true;
    emit colorChanged();
    if (parentItem()) parentItem()->markDirty();
}

void CanvasShape::setDim(bool dim) {
    if (m_style.hasDim() && m_style.dim() == dim) return;
    m_style.setDim(dim);
    emit dimChanged();
    if (parentItem()) parentItem()->markDirty();
}

void CanvasShape::setGradient(Gradient* gradient) {
    if (m_gradient.data() == gradient) return;
    if (m_gradient) {
        disconnect(m_gradient.data(), nullptr, this, nullptr);
    }
    m_gradient = gradient;
    if (m_gradient) {
        connect(m_gradient.data(), &Gradient::updated, this, [this]() {
            if (parentItem()) parentItem()->markDirty();
        });
        connect(m_gradient.data(), &QObject::destroyed, this, [this]() {
            m_gradient = nullptr;
            emit gradientChanged();
            if (parentItem()) parentItem()->markDirty();
        });
    }
    emit gradientChanged();
    if (parentItem()) parentItem()->markDirty();
}

uint32_t CanvasShape::resolvedColor(const CanvasPainter& painter) const {
    if (m_gradient && !m_gradient->stopsList().isEmpty()) {
        return Color::paintedArgb32(m_gradient->colorArgbAt(0.0));
    }
    if (m_hasCustomColor) return m_resolvedColor;
    return painter.defaultColor();
}

uint32_t CanvasShape::resolvePixelColor(const CanvasPainter& painter, int px, int py, int minX,
                                        int minY, int maxX, int maxY) const {
    if (m_gradient && !m_gradient->stopsList().isEmpty()) {
        qreal t = 0.0;
        if (m_gradient->orientation() == Gradient::Horizontal) {
            const int span = maxX - minX;
            t = (span > 0) ? static_cast<qreal>(px - minX) / span : 0.0;
        } else {
            const int span = maxY - minY;
            t = (span > 0) ? static_cast<qreal>(py - minY) / span : 0.0;
        }
        return Color::paintedArgb32(m_gradient->colorArgbAt(t));
    }
    if (m_hasCustomColor) return m_resolvedColor;
    return painter.defaultColorAt(px, py);
}

Point::Point(QObject* parent) : CanvasShape(parent) {}

void Point::draw(CanvasPainter& painter) const {
    painter.setPixelColorHex(x(), y(), resolvePixelColor(painter, x(), y(), x(), y(), x(), y()));
}

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
    const int minX = std::min(m_x1, m_x2);
    const int maxX = std::max(m_x1, m_x2);
    const int minY = std::min(m_y1, m_y2);
    const int maxY = std::max(m_y1, m_y2);
    drawBresenhamLine(painter, *this, m_x1, m_y1, m_x2, m_y2, minX, minY, maxX, maxY);
}

Polygon::Polygon(QObject* parent) : CanvasShape(parent) {}

void Polygon::setPoints(const QVariantList& points) {
    QList<QPoint> parsed;
    parsed.reserve(points.size());
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
        } else if (var.canConvert<QPoint>()) {
            const QPoint pt = var.toPoint();
            px = pt.x();
            py = pt.y();
        }
        parsed.append(QPoint(px, py));
    }
    if (m_points == points && m_parsedPoints == parsed) return;
    m_parsedPoints = std::move(parsed);
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

    int minX = m_parsedPoints.front().x();
    int maxX = minX;
    int minY = m_parsedPoints.front().y();
    int maxY = minY;
    for (const QPoint& pt : m_parsedPoints) {
        minX = std::min(minX, pt.x());
        maxX = std::max(maxX, pt.x());
        minY = std::min(minY, pt.y());
        maxY = std::max(maxY, pt.y());
    }

    for (int i = 0; i < m_parsedPoints.size() - 1; ++i) {
        drawBresenhamLine(painter, *this, m_parsedPoints[i].x(), m_parsedPoints[i].y(),
                          m_parsedPoints[i + 1].x(), m_parsedPoints[i + 1].y(), minX, minY, maxX,
                          maxY);
    }

    if (m_closed && m_parsedPoints.size() > 2) {
        drawBresenhamLine(painter, *this, m_parsedPoints.back().x(), m_parsedPoints.back().y(),
                          m_parsedPoints.front().x(), m_parsedPoints.front().y(), minX, minY, maxX,
                          maxY);
    }
}

CanvasRectangle::CanvasRectangle(QObject* parent) : CanvasShape(parent) {}

void CanvasRectangle::setFill(bool fill) {
    if (m_fill != fill) {
        m_fill = fill;
        emit fillChanged();
        if (parentItem()) parentItem()->markDirty();
    }
}

void CanvasRectangle::draw(CanvasPainter& painter) const {
    const int rx = x();
    const int ry = y();
    const int rw = width();
    const int rh = height();

    if (rw <= 0 || rh <= 0) return;

    const int minX = rx;
    const int minY = ry;
    const int maxX = rx + rw - 1;
    const int maxY = ry + rh - 1;

    if (m_fill) {
        for (int py = ry; py < ry + rh; ++py) {
            for (int px = rx; px < rx + rw; ++px) {
                painter.setPixelColorHex(
                    px, py, resolvePixelColor(painter, px, py, minX, minY, maxX, maxY));
            }
        }
    } else {
        drawBresenhamLine(painter, *this, rx, ry, maxX, ry, minX, minY, maxX, maxY);
        drawBresenhamLine(painter, *this, rx, maxY, maxX, maxY, minX, minY, maxX, maxY);
        drawBresenhamLine(painter, *this, rx, ry, rx, maxY, minX, minY, maxX, maxY);
        drawBresenhamLine(painter, *this, maxX, ry, maxX, maxY, minX, minY, maxX, maxY);
    }
}

Circle::Circle(QObject* parent) : CanvasShape(parent) {}

void Circle::setCx(int cx) {
    if (m_cxSet && m_cx == cx) return;
    m_cx = cx;
    m_cxSet = true;
    emit cxChanged();
    if (parentItem()) parentItem()->markDirty();
}
void Circle::setCy(int cy) {
    if (m_cySet && m_cy == cy) return;
    m_cy = cy;
    m_cySet = true;
    emit cyChanged();
    if (parentItem()) parentItem()->markDirty();
}
void Circle::setRadius(int r) {
    if (m_radius != r) {
        m_radius = r;
        emit radiusChanged();
        if (parentItem()) parentItem()->markDirty();
    }
}
void Circle::setFill(bool fill) {
    if (m_fill != fill) {
        m_fill = fill;
        emit fillChanged();
        if (parentItem()) parentItem()->markDirty();
    }
}

void Circle::draw(CanvasPainter& painter) const {
    const int r = m_radius;
    if (r <= 0) return;

    const int centerx = cx();
    const int centery = cy();
    const int minX = centerx - r;
    const int maxX = centerx + r;
    const int minY = centery - r;
    const int maxY = centery + r;
    int x = r;
    int y = 0;
    // Midpoint decision between the next vertical and diagonal pixel steps.
    int err = 1 - r;

    auto setShapePixel = [&](int px, int py) {
        painter.setPixelColorHex(px, py,
                                 resolvePixelColor(painter, px, py, minX, minY, maxX, maxY));
    };

    auto drawPoints = [&](int pcx, int pcy, int px, int py) {
        if (m_fill) {
            for (int ix = pcx - px; ix <= pcx + px; ++ix) {
                setShapePixel(ix, pcy + py);
                setShapePixel(ix, pcy - py);
            }
            for (int ix = pcx - py; ix <= pcx + py; ++ix) {
                setShapePixel(ix, pcy + px);
                setShapePixel(ix, pcy - px);
            }
        } else {
            setShapePixel(pcx + px, pcy + py);
            setShapePixel(pcx - px, pcy + py);
            setShapePixel(pcx + px, pcy - py);
            setShapePixel(pcx - px, pcy - py);
            setShapePixel(pcx + py, pcy + px);
            setShapePixel(pcx - py, pcy + px);
            setShapePixel(pcx + py, pcy - px);
            setShapePixel(pcx - py, pcy - px);
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

} // namespace qmltui
