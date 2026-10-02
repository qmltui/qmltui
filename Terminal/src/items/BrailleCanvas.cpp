#include "BrailleCanvas.h"
#include "ColorMapper.h"

namespace qtui {

static void drawBresenhamLine(BrailleCanvas* canvas, int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    int x = x1, y = y1;
    while (true) {
        canvas->setPixel(x, y);
        if (x == x2 && y == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

// --- BrailleCanvas ---
BrailleCanvas::BrailleCanvas(QObject* parent) : Item(parent) {
    resizeBuffer();
}

void BrailleCanvas::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void BrailleCanvas::setPixel(int x, int y, bool value) {
    int cellX = x / 2;
    int cellY = y / 4;
    if (cellX < 0 || cellX >= width() || cellY < 0 || cellY >= height()) return;

    int subX = x % 2;
    int subY = y % 4;
    
    // Map sub-coordinates to 0-7 bit position
    int bitIndex = 0;
    if (subY < 3) {
        bitIndex = subX * 3 + subY;
    } else { // Bottom row (subY == 3)
        bitIndex = 6 + subX;
    }

    int cellIndex = cellY * width() + cellX;
    if (value) {
        m_cells[cellIndex] |= (1 << bitIndex);
    } else {
        m_cells[cellIndex] &= ~(1 << bitIndex);
    }
    markDirty();
}

void BrailleCanvas::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible()) return;

    resizeBuffer();
    std::fill(m_cells.begin(), m_cells.end(), 0x00);

    // Draw all visual children shapes
    for (Item* child : childItems()) {
        if (child && child->visible()) {
            if (BrailleShape* shape = qobject_cast<BrailleShape*>(child)) {
                shape->draw(this);
            }
        }
    }

    // Render cells as Braille characters
    for (int y = 0; y < height(); ++y) {
        for (int x = 0; x < width(); ++x) {
            uint8_t cellValue = m_cells[y * width() + x];
            if (cellValue > 0) {
                // Braille patterns start at Unicode U+2800 (offset 10240)
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
}

void BrailleCanvas::resizeBuffer() {
    size_t needed = width() * height();
    if (m_cells.size() != needed) {
        m_cells.resize(needed, 0x00);
    }
}

// --- BraillePoint ---
BraillePoint::BraillePoint(QObject* parent) : BrailleShape(parent) {}

void BraillePoint::setX(int x) {
    if (m_x == x) return;
    m_x = x;
    emit xChanged();
    if (parentItem()) parentItem()->markDirty();
}

void BraillePoint::setY(int y) {
    if (m_y == y) return;
    m_y = y;
    emit yChanged();
    if (parentItem()) parentItem()->markDirty();
}

void BraillePoint::draw(BrailleCanvas* canvas) {
    canvas->setPixel(m_x, m_y);
}

// --- BrailleLine ---
BrailleLine::BrailleLine(QObject* parent) : BrailleShape(parent) {}

void BrailleLine::setX1(int x1) {
    if (m_x1 == x1) return;
    m_x1 = x1;
    emit x1Changed();
    if (parentItem()) parentItem()->markDirty();
}

void BrailleLine::setY1(int y1) {
    if (m_y1 == y1) return;
    m_y1 = y1;
    emit y1Changed();
    if (parentItem()) parentItem()->markDirty();
}

void BrailleLine::setX2(int x2) {
    if (m_x2 == x2) return;
    m_x2 = x2;
    emit x2Changed();
    if (parentItem()) parentItem()->markDirty();
}

void BrailleLine::setY2(int y2) {
    if (m_y2 == y2) return;
    m_y2 = y2;
    emit y2Changed();
    if (parentItem()) parentItem()->markDirty();
}

void BrailleLine::draw(BrailleCanvas* canvas) {
    drawBresenhamLine(canvas, m_x1, m_y1, m_x2, m_y2);
}

// --- BraillePolygon ---
BraillePolygon::BraillePolygon(QObject* parent) : BrailleShape(parent) {}

void BraillePolygon::setPoints(const QVariantList& points) {
    m_points = points;
    emit pointsChanged();
    if (parentItem()) parentItem()->markDirty();
}

void BraillePolygon::setClosed(bool closed) {
    if (m_closed == closed) return;
    m_closed = closed;
    emit closedChanged();
    if (parentItem()) parentItem()->markDirty();
}

void BraillePolygon::draw(BrailleCanvas* canvas) {
    if (m_points.isEmpty()) return;

    std::vector<std::pair<int, int>> coords;
    for (const QVariant& var : m_points) {
        QVariantMap map = var.toMap();
        coords.push_back({ map["x"].toInt(), map["y"].toInt() });
    }

    for (size_t i = 0; i < coords.size() - 1; ++i) {
        drawBresenhamLine(canvas, coords[i].first, coords[i].second, coords[i + 1].first, coords[i + 1].second);
    }

    if (m_closed && coords.size() > 2) {
        drawBresenhamLine(canvas, coords.back().first, coords.back().second, coords.front().first, coords.front().second);
    }
}

} // namespace qtui
