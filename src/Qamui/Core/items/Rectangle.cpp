#include <Qamui/Core/items/Rectangle.h>
#include <Qamui/Core/items/Gradient.h>
#include <Qamui/Core/primitives/ColorMapper.h>

namespace qamui {

Rectangle::Rectangle(QObject *parent) : Item(parent), m_border(this) {}

Rectangle::~Rectangle() = default;

void Rectangle::setColor(Color color) {
  if (m_color == color)
    return;
  m_color = color;
  m_resolvedBackgroundColor = ColorMapper::fromString(m_color, Color::TransparentTag);
  emit colorChanged();
  markDirty();
}

void Rectangle::setGradient(Gradient *gradient) {
  if (m_gradient.data() == gradient)
    return;
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

int Rectangle::leftPadding() const {
  int p = Item::leftPadding();
  if (m_border.style() != "none")
    p += 1;
  return p;
}

int Rectangle::rightPadding() const {
  int p = Item::rightPadding();
  if (m_border.style() != "none")
    p += 1;
  return p;
}

int Rectangle::topPadding() const {
  int p = Item::topPadding();
  if (m_border.style() != "none")
    p += 1;
  return p;
}

int Rectangle::bottomPadding() const {
  int p = Item::bottomPadding();
  if (m_border.style() != "none")
    p += 1;
  return p;
}

void Rectangle::paint(TextGridBuffer &buffer, int absX, int absY,
                      const ClipRect &clipRect) const {
  if (!visible())
    return;

  int w = width();
  int h = height();

  const bool hasGradient = (m_gradient && !m_gradient->stopsList().isEmpty());
  // Anything but explicit transparency is painted, including the terminal's
  // own default colour: a surface has to be written in order to occlude what
  // it covers, even when the terminal is the one choosing the colour.
  const bool hasSolidBg = !ColorMapper::isTransparent(m_resolvedBackgroundColor);

  // Render background if specified via gradient or solid color
  if (hasGradient || hasSolidBg) {
    const bool isHorizontal = (m_gradient && m_gradient->orientation() == Gradient::Horizontal);
    for (int dy = 0; dy < h; ++dy) {
      int cy = absY + dy;
      for (int dx = 0; dx < w; ++dx) {
        int cx = absX + dx;
        if (clipRect.contains(cx, cy)) {
          if (cx >= 0 && cx < buffer.width() && cy >= 0 &&
              cy < buffer.height()) {
            Cell &cell = buffer.cellAt(cx, cy);
            if (hasGradient) {
              qreal t = 0.0;
              if (isHorizontal) {
                t = (w > 1) ? static_cast<qreal>(dx) / (w - 1) : 0.0;
              } else {
                t = (h > 1) ? static_cast<qreal>(dy) / (h - 1) : 0.0;
              }
              cell.bgColor = m_gradient->colorAt(t);
            } else {
              cell.bgColor = m_resolvedBackgroundColor;
            }
            cell.setCharacter(" ");
            cell.displayWidth = 1;
            cell.continuation = false;
            cell.attributes = TextAttribute::None;
          }
        }
      }
    }
  }

  // Render border if enabled
  if (m_border.style() != "none") {
    uint32_t borderColor =
        ColorMapper::fromString(m_border.color(), 0xFFFFFFFF);
    const char *tl = "┌";
    const char *tr = "┐";
    const char *bl = "└";
    const char *br = "┘";
    const char *hz = "─";
    const char *vt = "│";

    if (m_border.style() == "double") {
      tl = "╔";
      tr = "╗";
      bl = "╚";
      br = "╝";
      hz = "═";
      vt = "║";
    } else if (m_border.style() == "ascii") {
      tl = "+";
      tr = "+";
      bl = "+";
      br = "+";
      hz = "-";
      vt = "|";
    } else if (m_border.style() == "rounded") {
      tl = "╭";
      tr = "╮";
      bl = "╰";
      br = "╯";
    }

    // Measure the six glyphs once instead of per border cell.
    const QByteArrayView tlGlyph(tl), trGlyph(tr), blGlyph(bl), brGlyph(br),
        hzGlyph(hz), vtGlyph(vt);
    auto setCellChar = [](Cell &cell, QByteArrayView glyph, uint32_t fg) {
      cell.setCharacter(glyph);
      cell.displayWidth = 1;
      cell.continuation = false;
      cell.fgColor = fg;
      cell.attributes = TextAttribute::None;
    };

    for (int dy = 0; dy < h; ++dy) {
      int cy = absY + dy;
      for (int dx = 0; dx < w; ++dx) {
        int cx = absX + dx;
        if (clipRect.contains(cx, cy)) {
          if (cx >= 0 && cx < buffer.width() && cy >= 0 &&
              cy < buffer.height()) {
            bool isBorder = (dx == 0 || dx == w - 1 || dy == 0 || dy == h - 1);
            if (isBorder) {
              Cell &cell = buffer.cellAt(cx, cy);
              if (dx == 0 && dy == 0)
                setCellChar(cell, tlGlyph, borderColor);
              else if (dx == w - 1 && dy == 0)
                setCellChar(cell, trGlyph, borderColor);
              else if (dx == 0 && dy == h - 1)
                setCellChar(cell, blGlyph, borderColor);
              else if (dx == w - 1 && dy == h - 1)
                setCellChar(cell, brGlyph, borderColor);
              else if (dy == 0 || dy == h - 1)
                setCellChar(cell, hzGlyph, borderColor);
              else if (dx == 0 || dx == w - 1)
                setCellChar(cell, vtGlyph, borderColor);
            }
          }
        }
      }
    }
  }

  // Child traversal is performed by Item after this hook returns.
}

} // namespace qamui
