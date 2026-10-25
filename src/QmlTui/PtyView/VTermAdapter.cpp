#include <QmlTui/PtyView/VTermAdapter.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>

#include <Qt>

#include <algorithm>
#include <cstring>

namespace qmltui {

namespace {

uint8_t colorChannel(Color color, int shift) {
    return static_cast<uint8_t>((color.toRgb().argb32() >> shift) & 0xffU);
}

const VTermScreenCallbacks ScreenCallbacks{
    &VTermAdapter::damage,
    &VTermAdapter::moveRect,
    &VTermAdapter::moveCursor,
    &VTermAdapter::setTermProperty,
    &VTermAdapter::bell,
    &VTermAdapter::resized,
    &VTermAdapter::pushScrollback,
    &VTermAdapter::popScrollback,
    &VTermAdapter::clearScrollbackCallback,
};

const VTermSelectionCallbacks SelectionCallbacks{
    &VTermAdapter::setSelection,
    &VTermAdapter::querySelection,
};

const VTermStateFallbacks StateFallbacks{
    nullptr, nullptr, &VTermAdapter::unknownOsc, nullptr, nullptr, nullptr, nullptr,
};

TextAttribute attributesFor(const VTermScreenCell& cell) {
    TextAttribute attributes = TextAttribute::None;
    if (cell.attrs.bold) attributes = attributes | TextAttribute::Bold;
    if (cell.attrs.italic) attributes = attributes | TextAttribute::Italic;
    if (cell.attrs.underline) attributes = attributes | TextAttribute::Underline;
    if (cell.attrs.reverse) attributes = attributes | TextAttribute::Reversed;
    if (cell.attrs.blink) attributes = attributes | TextAttribute::Blink;
    if (cell.attrs.conceal) attributes = attributes | TextAttribute::Conceal;
    if (cell.attrs.strike) attributes = attributes | TextAttribute::Strikethrough;
    return attributes;
}

} // namespace

VTermAdapter::VTermAdapter(int rows, int columns)
    : m_rows((std::max)(1, rows)), m_columns((std::max)(1, columns)) {
    VTermBuilder builder{};
    builder.rows = m_rows;
    builder.cols = m_columns;
    builder.outbuffer_len = 4096;
    builder.tmpbuffer_len = 64 * 1024;
    m_vterm = vterm_build(&builder);
    if (!m_vterm) return;

    vterm_set_utf8(m_vterm, 1);
    vterm_output_set_callback(m_vterm, &VTermAdapter::outputBytes, this);
    m_state = vterm_obtain_state(m_vterm);
    m_screen = vterm_obtain_screen(m_vterm);
    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_enable_reflow(m_screen, true);
    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_ROW);
    vterm_screen_set_callbacks(m_screen, &ScreenCallbacks, this);
    vterm_state_set_unrecognised_fallbacks(m_state, &StateFallbacks, this);
    m_selectionBuffer.resize(1024 * 1024);
    vterm_state_set_selection_callbacks(m_state, &SelectionCallbacks, this,
                                        m_selectionBuffer.data(),
                                        static_cast<size_t>(m_selectionBuffer.size()));
    vterm_screen_reset(m_screen, 1);
}

VTermAdapter::~VTermAdapter() {
    if (m_vterm) vterm_free(m_vterm);
}

void VTermAdapter::feed(const QByteArray& bytes) {
    if (!m_vterm || bytes.isEmpty()) return;
    vterm_input_write(m_vterm, bytes.constData(), static_cast<size_t>(bytes.size()));
    vterm_screen_flush_damage(m_screen);
}

void VTermAdapter::resize(int rows, int columns) {
    rows = (std::max)(1, rows);
    columns = (std::max)(1, columns);
    if (!m_vterm || (rows == m_rows && columns == m_columns)) return;
    m_rows = rows;
    m_columns = columns;
    vterm_set_size(m_vterm, rows, columns);
    vterm_screen_flush_damage(m_screen);
}

void VTermAdapter::setDefaultColors(Color foreground, Color background) {
    if (!m_state || !foreground.isValid() || !background.isValid()) return;
    VTermColor fg;
    VTermColor bg;
    vterm_color_rgb(&fg, colorChannel(foreground, 16), colorChannel(foreground, 8),
                    colorChannel(foreground, 0));
    vterm_color_rgb(&bg, colorChannel(background, 16), colorChannel(background, 8),
                    colorChannel(background, 0));
    vterm_state_set_default_colors(m_state, &fg, &bg);
    m_defaultForeground = foreground.argb32();
    m_defaultBackground = background.argb32();
}

void VTermAdapter::setPaletteColor(int index, Color color) {
    if (!m_state || index < 0 || index >= 16 || !color.isValid()) return;
    VTermColor c;
    vterm_color_rgb(&c, colorChannel(color, 16), colorChannel(color, 8), colorChannel(color, 0));
    vterm_state_set_palette_color(m_state, index, &c);
}

void VTermAdapter::setPalette(const QList<Color>& palette) {
    m_palette = palette;
    for (int i = 0; i < (std::min)(16, static_cast<int>(palette.size())); ++i) {
        setPaletteColor(i, palette[i]);
    }
    if (m_damageCallback) m_damageCallback();
}

void VTermAdapter::setScrollbackLimit(int limit) {
    m_scrollbackLimit = (std::max)(0, limit);
    while (static_cast<int>(m_scrollback.size()) > m_scrollbackLimit)
        m_scrollback.pop_front();
}

void VTermAdapter::clearScrollback() {
    m_scrollback.clear();
    if (m_damageCallback) m_damageCallback();
}

void VTermAdapter::reset() {
    if (!m_vterm) return;
    if (m_state) {
        vterm_state_reset(m_state, 1);
        if (!m_palette.isEmpty()) {
            for (int i = 0; i < (std::min)(16, static_cast<int>(m_palette.size())); ++i)
                setPaletteColor(i, m_palette[i]);
        }
        VTermColor fg;
        VTermColor bg;
        vterm_color_rgb(&fg, static_cast<uint8_t>((m_defaultForeground >> 16) & 0xff),
                        static_cast<uint8_t>((m_defaultForeground >> 8) & 0xff),
                        static_cast<uint8_t>(m_defaultForeground & 0xff));
        vterm_color_rgb(&bg, static_cast<uint8_t>((m_defaultBackground >> 16) & 0xff),
                        static_cast<uint8_t>((m_defaultBackground >> 8) & 0xff),
                        static_cast<uint8_t>(m_defaultBackground & 0xff));
        vterm_state_set_default_colors(m_state, &fg, &bg);
    }
    if (m_screen) {
        vterm_screen_reset(m_screen, 1);
        vterm_screen_flush_damage(m_screen);
    }
    m_scrollback.clear();
    if (m_damageCallback) m_damageCallback();
}

void VTermAdapter::sendKey(int key, const QString& text, int modifiers) {
    if (!m_vterm) return;
    const VTermModifier vtermModifiers = toVTermModifiers(modifiers);
    if (!text.isEmpty()) {
        const QList<uint> codepoints = text.toUcs4();
        for (uint codepoint : codepoints)
            vterm_keyboard_unichar(m_vterm, codepoint, vtermModifiers);
        return;
    }
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        vterm_keyboard_unichar(m_vterm, static_cast<uint32_t>('a' + key - Qt::Key_A),
                               vtermModifiers);
        return;
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        vterm_keyboard_unichar(m_vterm, static_cast<uint32_t>('0' + key - Qt::Key_0),
                               vtermModifiers);
        return;
    }
    const VTermKey vtermKey = toVTermKey(key);
    if (vtermKey != VTERM_KEY_NONE) vterm_keyboard_key(m_vterm, vtermKey, vtermModifiers);
}

void VTermAdapter::sendPaste(const QString& text) {
    if (!m_vterm) return;
    vterm_keyboard_start_paste(m_vterm);
    if (m_outputCallback) m_outputCallback(text.toUtf8());
    vterm_keyboard_end_paste(m_vterm);
}

void VTermAdapter::sendMouseMove(int row, int column, int modifiers) {
    if (m_vterm) vterm_mouse_move(m_vterm, row, column, toVTermModifiers(modifiers));
}

void VTermAdapter::sendMouseButton(int button, bool pressed, int modifiers) {
    if (m_vterm) vterm_mouse_button(m_vterm, button + 1, pressed, toVTermModifiers(modifiers));
}

void VTermAdapter::sendWheel(int deltaY, int modifiers) {
    if (!m_vterm || deltaY == 0) return;
    const int button = deltaY > 0 ? 4 : 5;
    const int count = std::abs(deltaY);
    for (int index = 0; index < count; ++index) {
        vterm_mouse_button(m_vterm, button, true, toVTermModifiers(modifiers));
        vterm_mouse_button(m_vterm, button, false, toVTermModifiers(modifiers));
    }
}

void VTermAdapter::setFocused(bool focused) {
    if (!m_state) return;
    if (focused)
        vterm_state_focus_in(m_state);
    else
        vterm_state_focus_out(m_state);
}

void VTermAdapter::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clip,
                          int width, int height, int scrollOffset) const {
    if (!m_screen) return;
    const int historyRows = m_altScreen ? 0 : static_cast<int>(m_scrollback.size());
    const int effectiveOffset = m_altScreen ? 0 : std::clamp(scrollOffset, 0, historyRows);
    const int viewStart = historyRows - effectiveOffset;

    for (int row = 0; row < height; ++row) {
        const int sourceRow = viewStart + row;
        for (int column = 0; column < width; ++column) {
            const int targetX = absX + column;
            const int targetY = absY + row;
            if (!clip.contains(targetX, targetY)) continue;

            VTermScreenCell source{};
            bool hasSource = false;
            if (sourceRow < historyRows) {
                const auto& line = m_scrollback[static_cast<size_t>(sourceRow)];
                if (column < static_cast<int>(line.size())) {
                    source = line[static_cast<size_t>(column)];
                    hasSource = true;
                }
            } else {
                const int screenRow = sourceRow - historyRows;
                if (screenRow >= 0 && screenRow < m_rows && column < m_columns) {
                    source = screenCell(screenRow, column);
                    hasSource = true;
                }
            }

            Cell* targetCell = buffer.tryCellAt(targetX, targetY);
            if (!targetCell) continue;
            Cell& target = *targetCell;
            target.fgColor = hasSource ? colorToArgb(source.fg) : m_defaultForeground;
            target.bgColor = hasSource ? colorToArgb(source.bg) : m_defaultBackground;
            target.attributes = attributesFor(source);
            target.displayWidth =
                hasSource ? static_cast<uint8_t>((std::max)(1, static_cast<int>(source.width))) : 1;
            target.continuation = hasSource && (source.width == 0);

            if (!hasSource || source.attrs.conceal) {
                target.setCharacter(QByteArrayView(" ", 1));
            } else {
                char utf8[24];
                int utf8Len = 0;
                for (uint32_t codepoint : source.chars) {
                    if (codepoint == 0) break;
                    utf8Len += UnicodeText::encodeCodePointUtf8(codepoint, utf8 + utf8Len);
                }
                if (utf8Len == 0)
                    target.setCharacter(QByteArrayView(" ", 1));
                else
                    target.setCharacter(QByteArrayView(utf8, utf8Len));
            }
        }
    }

    if (effectiveOffset == 0 && m_cursorVisible && m_cursor.row >= 0 && m_cursor.row < height &&
        m_cursor.col >= 0 && m_cursor.col < width) {
        const int cursorX = absX + m_cursor.col;
        const int cursorY = absY + m_cursor.row;
        if (clip.contains(cursorX, cursorY))
            if (Cell* cursorCell = buffer.tryCellAt(cursorX, cursorY))
                cursorCell->attributes = cursorCell->attributes | TextAttribute::Reversed;
    }
}

int VTermAdapter::damage(VTermRect, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (self->m_damageCallback) self->m_damageCallback();
    return 1;
}

int VTermAdapter::moveRect(VTermRect, VTermRect, void* user) {
    return damage({}, user);
}

int VTermAdapter::moveCursor(VTermPos pos, VTermPos, int visible, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    self->m_cursor = pos;
    self->m_cursorVisible = visible != 0;
    if (self->m_damageCallback) self->m_damageCallback();
    return 1;
}

int VTermAdapter::setTermProperty(VTermProp prop, VTermValue* value, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    switch (prop) {
    case VTERM_PROP_TITLE:
        self->appendFragment(self->m_titleBuffer, value->string, 4096, self->m_titleCallback);
        break;
    case VTERM_PROP_CURSORVISIBLE:
        self->m_cursorVisible = value->boolean != 0;
        break;
    case VTERM_PROP_MOUSE:
        self->m_mouseMode = value->number;
        if (self->m_mouseModeCallback) self->m_mouseModeCallback(self->m_mouseMode);
        break;
    case VTERM_PROP_ALTSCREEN:
        self->m_altScreen = value->boolean != 0;
        break;
    default:
        break;
    }
    if (self->m_damageCallback) self->m_damageCallback();
    return 1;
}

int VTermAdapter::bell(void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (self->m_bellCallback) self->m_bellCallback();
    return 1;
}

int VTermAdapter::resized(int rows, int columns, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    self->m_rows = rows;
    self->m_columns = columns;
    return damage({}, user);
}

int VTermAdapter::pushScrollback(int columns, const VTermScreenCell* cells, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (self->m_altScreen || self->m_scrollbackLimit == 0) return 1;
    self->m_scrollback.emplace_back(cells, cells + columns);
    while (static_cast<int>(self->m_scrollback.size()) > self->m_scrollbackLimit)
        self->m_scrollback.pop_front();
    if (self->m_damageCallback) self->m_damageCallback();
    return 1;
}

int VTermAdapter::popScrollback(int columns, VTermScreenCell* cells, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (self->m_scrollback.empty()) return 0;
    const auto line = std::move(self->m_scrollback.back());
    self->m_scrollback.pop_back();
    std::memset(cells, 0, sizeof(VTermScreenCell) * static_cast<size_t>(columns));
    std::copy_n(line.begin(), (std::min)(columns, static_cast<int>(line.size())), cells);
    return 1;
}

int VTermAdapter::clearScrollbackCallback(void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    self->m_scrollback.clear();
    if (self->m_damageCallback) self->m_damageCallback();
    return 1;
}

int VTermAdapter::setSelection(VTermSelectionMask mask, VTermStringFragment fragment, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if ((mask & VTERM_SELECTION_CLIPBOARD) && self->m_clipboardCallback)
        self->appendFragment(self->m_clipboardBuffer, fragment, 1024 * 1024,
                             self->m_clipboardCallback);
    return 1;
}

int VTermAdapter::querySelection(VTermSelectionMask, void*) {
    return 1;
}

int VTermAdapter::unknownOsc(int command, VTermStringFragment fragment, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (command != 9 && command != 777) return 0;
    self->appendFragment(self->m_notificationBuffer, fragment, 64 * 1024,
                         self->m_notificationCallback);
    return 1;
}

void VTermAdapter::outputBytes(const char* bytes, size_t length, void* user) {
    auto* self = static_cast<VTermAdapter*>(user);
    if (self->m_outputCallback)
        self->m_outputCallback(QByteArray(bytes, static_cast<qsizetype>(length)));
}

VTermModifier VTermAdapter::toVTermModifiers(int modifiers) {
    int result = VTERM_MOD_NONE;
    if (modifiers & Qt::ShiftModifier) result |= VTERM_MOD_SHIFT;
    if (modifiers & Qt::AltModifier) result |= VTERM_MOD_ALT;
    if (modifiers & Qt::ControlModifier) result |= VTERM_MOD_CTRL;
    return static_cast<VTermModifier>(result);
}

VTermKey VTermAdapter::toVTermKey(int key) {
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return VTERM_KEY_ENTER;
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        return VTERM_KEY_TAB;
    case Qt::Key_Backspace:
        return VTERM_KEY_BACKSPACE;
    case Qt::Key_Escape:
        return VTERM_KEY_ESCAPE;
    case Qt::Key_Up:
        return VTERM_KEY_UP;
    case Qt::Key_Down:
        return VTERM_KEY_DOWN;
    case Qt::Key_Left:
        return VTERM_KEY_LEFT;
    case Qt::Key_Right:
        return VTERM_KEY_RIGHT;
    case Qt::Key_Insert:
        return VTERM_KEY_INS;
    case Qt::Key_Delete:
        return VTERM_KEY_DEL;
    case Qt::Key_Home:
        return VTERM_KEY_HOME;
    case Qt::Key_End:
        return VTERM_KEY_END;
    case Qt::Key_PageUp:
        return VTERM_KEY_PAGEUP;
    case Qt::Key_PageDown:
        return VTERM_KEY_PAGEDOWN;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F35)
            return static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + key - Qt::Key_F1 + 1);
        return VTERM_KEY_NONE;
    }
}

uint32_t VTermAdapter::colorToArgb(VTermColor color) const {
    if (!m_screen) return 0;
    if (VTERM_COLOR_IS_DEFAULT_FG(&color)) return m_defaultForeground;
    if (VTERM_COLOR_IS_DEFAULT_BG(&color)) return m_defaultBackground;
    vterm_screen_convert_color_to_rgb(m_screen, &color);
    return 0xff000000u | (static_cast<uint32_t>(color.rgb.red) << 16) |
           (static_cast<uint32_t>(color.rgb.green) << 8) | static_cast<uint32_t>(color.rgb.blue);
}

VTermScreenCell VTermAdapter::screenCell(int row, int column) const {
    VTermScreenCell cell{};
    vterm_screen_get_cell(m_screen, VTermPos{row, column}, &cell);
    return cell;
}

void VTermAdapter::appendFragment(QByteArray& target, VTermStringFragment fragment, qsizetype limit,
                                  const StringCallback& callback) {
    if (fragment.initial) target.clear();
    const qsizetype available = (std::max)(qsizetype(0), limit - target.size());
    target.append(fragment.str, (std::min)(available, static_cast<qsizetype>(fragment.len)));
    if (fragment.final) {
        if (callback) callback(QString::fromUtf8(target));
        target.clear();
    }
}

} // namespace qmltui
