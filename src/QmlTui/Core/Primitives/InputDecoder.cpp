#include <QmlTui/Core/Primitives/InputDecoder.h>
#include <QList>
#include <Qt>
#include <string_view>
#include <utility>
#include <limits>

namespace qmltui {

namespace {

using Sequence = InputDecoder::ControlSequence;

enum class ParseResult { Complete, Incomplete, Invalid, Oversized };

bool isFinalByte(unsigned char c);

ParseResult parseSequence(std::string_view bytes, Sequence& sequence, size_t& consumed,
                          size_t stringScanOffset) {
    const char introducer = bytes[1];
    sequence.kind = introducer == '['   ? Sequence::Csi
                    : introducer == 'O' ? Sequence::Ss3
                    : introducer == '_' ? Sequence::Apc
                    : introducer == ']' ? Sequence::Osc
                                        : Sequence::Dcs;
    const bool string = sequence.kind >= Sequence::Apc;
    const bool clipboard = bytes.rfind("\033]52;", 0) == 0;
    constexpr size_t ClipboardFrameLimit = 4 * ((InputDecoder::MaxClipboardTextBytes + 2) / 3) + 32;
    const size_t limit = clipboard ? ClipboardFrameLimit : string ? 4096 : 64;
    bool intermediates = false;
    size_t intermediateStart = 0;
    for (size_t pos = string ? stringScanOffset : 2; pos < bytes.size(); ++pos) {
        const unsigned char c = bytes[pos];
        consumed = pos + 1;
        if (c == 0x18 || c == 0x1a) return ParseResult::Invalid;
        if (c == 0x1b) {
            if (string) {
                if (pos + 1 == bytes.size()) {
                    consumed = pos;
                    return pos >= limit ? ParseResult::Oversized : ParseResult::Incomplete;
                }
                if (bytes[pos + 1] == '\\') {
                    consumed = pos + 2;
                    sequence.payload = QByteArrayView(bytes.data() + 2, pos - 2);
                    return consumed <= limit ? ParseResult::Complete : ParseResult::Invalid;
                }
            }
            consumed = pos; // restart at the new ESC
            return ParseResult::Invalid;
        }
        if (pos >= limit) {
            consumed = pos;
            return ParseResult::Oversized;
        }
        if (string) {
            if (sequence.kind == Sequence::Osc && c == 7) {
                sequence.payload = QByteArrayView(bytes.data() + 2, pos - 2);
                return ParseResult::Complete;
            }
            continue;
        }
        if (isFinalByte(c)) {
            sequence.finalByte = c;
            if (intermediates)
                sequence.intermediates =
                    QByteArrayView(bytes.data() + intermediateStart, pos - intermediateStart);
            return ParseResult::Complete;
        }
        if (c >= 0x20 && c <= 0x2f) {
            if (!intermediates) intermediateStart = pos;
            intermediates = true;
            continue;
        }
        if (c < 0x20 || c == 0x7f) {
            // A C0 control inside a CSI aborts it; the control is then decoded
            // on its own (Alt+[ followed by Enter still delivers Enter).
            consumed = pos;
            return ParseResult::Invalid;
        }
        if (intermediates) return ParseResult::Oversized; // discard malformed remainder
        if (pos == 2 && c >= '<' && c <= '?') {
            sequence.privateMarker = c;
            continue;
        }
        if (sequence.params.empty()) sequence.params.append({-1});
        if (c == ';')
            sequence.params.append({-1});
        else if (c == ':')
            sequence.params.last().append(-1);
        else if (c >= '0' && c <= '9') {
            int& value = sequence.params.last().last();
            if (value < 0) value = 0;
            if (value > ((std::numeric_limits<int>::max)() - (c - '0')) / 10)
                return ParseResult::Oversized;
            value = value * 10 + c - '0';
        } else
            return ParseResult::Oversized;
    }
    return ParseResult::Incomplete;
}

bool isFinalByte(unsigned char c) {
    return c >= 0x40 && c <= 0x7e;
}

// A string control (APC, DCS, OSC) is framed only when its first payload byte
// matches a reply QmlTui or a terminal query can produce. Otherwise ESC followed
// by ']', '_' or 'P' is an Alt-prefixed keystroke from a metaSendsEscape
// terminal and must never swallow the typing that follows it.
bool looksLikeReply(unsigned char introducer, unsigned char first) {
    switch (introducer) {
    case '_':
        return first == 'G'; // kitty graphics
    case 'P':
        return first == '$' || first == '>'     // DECRPSS, XTVERSION
               || first == '!' || first == '+'; // DECRPTUI, XTGETTCAP
    case ']':
        return first >= '0' && first <= '9'; // OSC colour and clipboard replies
    default:
        return false;
    }
}

// The modifier field shared by xterm (1 + Shift/Alt/Ctrl bits) and the kitty
// keyboard protocol, which extends it with Super (8), Hyper (16), Meta (32),
// CapsLock (64) and NumLock (128). Super and Meta both map to Qt::MetaModifier;
// Hyper and the lock bits have no Qt::KeyboardModifier and are dropped.
int keyModifiers(const Sequence& sequence) {
    const int value = sequence.parameter(1, 0, 1);
    if (value < 1) return 0;
    const int bits = value - 1;
    int modifiers = 0;
    if (bits & 1) modifiers |= Qt::ShiftModifier;
    if (bits & 2) modifiers |= Qt::AltModifier;
    if (bits & 4) modifiers |= Qt::ControlModifier;
    if (bits & (8 | 32)) modifiers |= Qt::MetaModifier;
    return modifiers;
}

// Kitty event type, the sub-parameter of the modifier field: 1 press
// (default), 2 repeat, 3 release. Anything else is unknown.
enum class KeyEventType { Press, Repeat, Release, Unknown };

KeyEventType keyEventType(const Sequence& sequence) {
    switch (sequence.parameter(1, 1, 1)) {
    case 1:
        return KeyEventType::Press;
    case 2:
        return KeyEventType::Repeat;
    case 3:
        return KeyEventType::Release;
    default:
        return KeyEventType::Unknown;
    }
}

bool isUnicodeScalar(int value) {
    return value >= 0 && value <= 0x10FFFF && (value < 0xD800 || value > 0xDFFF);
}

// Kitty functional keys occupy the Unicode private-use range 57344..63743.
// Keys without a Qt counterpart (media keys, the modifier keys themselves,
// which only arrive under flag 8) return 0 and are consumed silently.
struct FunctionalKey {
    int key = 0;
    char32_t text = 0;
    bool keypad = false;
};

FunctionalKey kittyFunctionalKey(int codepoint) {
    if (codepoint >= 57376 && codepoint <= 57398) return {Qt::Key_F13 + (codepoint - 57376)};
    if (codepoint >= 57399 && codepoint <= 57408)
        return {Qt::Key_0 + (codepoint - 57399), static_cast<char32_t>(U'0' + (codepoint - 57399)),
                true};
    switch (codepoint) {
    case 57358:
        return {Qt::Key_CapsLock};
    case 57359:
        return {Qt::Key_ScrollLock};
    case 57360:
        return {Qt::Key_NumLock};
    case 57361:
        return {Qt::Key_Print};
    case 57362:
        return {Qt::Key_Pause};
    case 57363:
        return {Qt::Key_Menu};
    case 57409:
        return {Qt::Key_Period, U'.', true};
    case 57410:
        return {Qt::Key_Slash, U'/', true};
    case 57411:
        return {Qt::Key_Asterisk, U'*', true};
    case 57412:
        return {Qt::Key_Minus, U'-', true};
    case 57413:
        return {Qt::Key_Plus, U'+', true};
    case 57414:
        return {Qt::Key_Enter, U'\n', true};
    case 57415:
        return {Qt::Key_Equal, U'=', true};
    case 57416:
        return {Qt::Key_Comma, U',', true};
    case 57417:
        return {Qt::Key_Left, 0, true};
    case 57418:
        return {Qt::Key_Right, 0, true};
    case 57419:
        return {Qt::Key_Up, 0, true};
    case 57420:
        return {Qt::Key_Down, 0, true};
    case 57421:
        return {Qt::Key_PageUp, 0, true};
    case 57422:
        return {Qt::Key_PageDown, 0, true};
    case 57423:
        return {Qt::Key_Home, 0, true};
    case 57424:
        return {Qt::Key_End, 0, true};
    case 57425:
        return {Qt::Key_Insert, 0, true};
    case 57426:
        return {Qt::Key_Delete, 0, true};
    case 57427:
        return {Qt::Key_Clear, 0, true};
    default:
        return {};
    }
}

} // namespace

InputDecoder::InputDecoder() {}

InputDecoder::~InputDecoder() {}

void InputDecoder::setKeyCallback(KeyCallback cb) {
    m_keyCallback = std::move(cb);
}

void InputDecoder::setPasteCallback(PasteCallback cb) {
    m_pasteCallback = std::move(cb);
}

void InputDecoder::setMouseCallback(MouseCallback cb) {
    m_mouseCallback = std::move(cb);
}

void InputDecoder::setDetailedMouseCallback(DetailedMouseCallback cb) {
    m_detailedMouseCallback = std::move(cb);
}

void InputDecoder::setFocusCallback(FocusCallback cb) {
    m_focusCallback = std::move(cb);
}

void InputDecoder::setReplyCallback(ReplyCallback cb) {
    m_replyCallback = std::move(cb);
}

void InputDecoder::setClipboardErrorCallback(std::function<void()> cb) {
    m_clipboardErrorCallback = std::move(cb);
}

void InputDecoder::dispatchSequence(const ControlSequence& sequence) {
    const char final = sequence.finalByte;
    // Inline rendering queries from column one after a nonempty prompt.
    // Its CPR cannot be a modified F3 key, whose first parameter is one.
    if (sequence.kind == Sequence::Csi && !sequence.privateMarker &&
        sequence.intermediates.isEmpty() && final == 'R' && sequence.params.size() == 2 &&
        sequence.params[0].size() == 1 && sequence.params[1].size() == 1 &&
        sequence.parameter(0) > 1 && sequence.parameter(1) == 1) {
        if (m_replyCallback) m_replyCallback(sequence);
        return;
    }
    if (sequence.kind == Sequence::Csi && sequence.privateMarker == '<' &&
        sequence.intermediates.isEmpty() && (final == 'M' || final == 'm')) {
        if (sequence.params.size() != 3) return;
        for (const auto& field : sequence.params)
            if (field.size() != 1 || field[0] < 0 || field[0] > 1'000'000) return;
        const int button = sequence.parameter(0);
        const int x = sequence.parameter(1) - 1;
        const int y = sequence.parameter(2) - 1;
        const bool release = final == 'm';
        const bool motion = (button & 32) != 0;
        if (m_mouseCallback) m_mouseCallback(button & ~32, x, y, release, motion);
        if (m_detailedMouseCallback) {
            MouseInput input;
            input.button = button & 3;
            input.x = x;
            input.y = y;
            input.release = release;
            input.motion = motion;
            if (button & 4) input.modifiers |= Qt::ShiftModifier;
            if (button & 8) input.modifiers |= Qt::AltModifier;
            if (button & 16) input.modifiers |= Qt::ControlModifier;
            if (button & 64) input.wheelDeltaY = (button & 1) ? -1 : 1;
            m_detailedMouseCallback(input);
        }
        return;
    }
    if (!sequence.privateMarker && sequence.intermediates.isEmpty() &&
        (sequence.kind == Sequence::Csi || sequence.kind == Sequence::Ss3)) {
        const int first = sequence.parameter(0);
        if (sequence.kind == Sequence::Csi && final == '~' && first == 200) {
            m_bracketedPaste = true;
            return;
        }
        if (sequence.kind == Sequence::Csi && sequence.params.isEmpty() &&
            (final == 'I' || final == 'O')) {
            if (m_focusCallback) m_focusCallback(final == 'I');
            return;
        }
        // Kitty event types apply to every key form once flag 2 is on; a
        // release or an unknown type is consumed and never dispatched.
        const KeyEventType eventType = keyEventType(sequence);
        if (eventType == KeyEventType::Release || eventType == KeyEventType::Unknown) return;
        if (sequence.kind == Sequence::Csi && final == 'u') {
            if (dispatchKittyKey(sequence)) return;
        }
        int key = 0;
        int modifiers = keyModifiers(sequence);
        if (final == 'Z' || (final == '~' && first == 27 && sequence.parameter(1) == 2 &&
                             sequence.parameter(2) == 9)) {
            key = Qt::Key_Backtab;
            modifiers = Qt::ShiftModifier;
        } else if (final == '~') {
            static constexpr std::pair<int, int> keys[] = {
                {1, Qt::Key_Home},   {2, Qt::Key_Insert},   {3, Qt::Key_Delete}, {4, Qt::Key_End},
                {5, Qt::Key_PageUp}, {6, Qt::Key_PageDown}, {7, Qt::Key_Home},   {8, Qt::Key_End},
                {11, Qt::Key_F1},    {12, Qt::Key_F2},      {13, Qt::Key_F3},    {14, Qt::Key_F4},
                {15, Qt::Key_F5},    {17, Qt::Key_F6},      {18, Qt::Key_F7},    {19, Qt::Key_F8},
                {20, Qt::Key_F9},    {21, Qt::Key_F10},     {23, Qt::Key_F11},   {24, Qt::Key_F12}};
            for (const auto& [parameter, value] : keys)
                if (first == parameter) {
                    key = value;
                    break;
                }
        } else {
            static constexpr std::pair<char, int> keys[] = {
                {'A', Qt::Key_Up},   {'B', Qt::Key_Down}, {'C', Qt::Key_Right}, {'D', Qt::Key_Left},
                {'H', Qt::Key_Home}, {'F', Qt::Key_End},  {'P', Qt::Key_F1},    {'Q', Qt::Key_F2},
                {'R', Qt::Key_F3},   {'S', Qt::Key_F4}};
            for (const auto& [character, value] : keys)
                if (final == character) {
                    key = value;
                    break;
                }
        }
        if (key) {
            if (m_keyCallback) m_keyCallback(key, QString(), modifiers);
            return;
        }
    }
    if (m_replyCallback) m_replyCallback(sequence);
}

// CSI unicode-key[:shifted-key[:base-layout-key]] ; modifiers[:event-type] [; text-codepoints] u
// The kitty keyboard protocol's key form. Returns false for a malformed
// sequence so it reaches the reply callback rather than the text path.
bool InputDecoder::dispatchKittyKey(const ControlSequence& sequence) {
    if (sequence.params.size() > 3) return false;
    const int codepoint = sequence.parameter(0, 0, -1);
    if (codepoint <= 0 || !isUnicodeScalar(codepoint)) return false;
    int modifiers = keyModifiers(sequence);
    const bool shortcut = modifiers & (Qt::ControlModifier | Qt::AltModifier);
    int key = 0;
    QString text;
    switch (codepoint) {
    case 27:
        key = Qt::Key_Escape;
        break;
    case 13:
        key = Qt::Key_Return;
        text = QStringLiteral("\n");
        break;
    case 9:
        key = (modifiers & Qt::ShiftModifier) ? Qt::Key_Backtab : Qt::Key_Tab;
        break;
    case 8:
    case 127:
        key = Qt::Key_Backspace;
        break;
    default:
        if (codepoint >= 57344 && codepoint <= 63743) {
            const FunctionalKey functional = kittyFunctionalKey(codepoint);
            if (!functional.key) return true; // media or modifier key: consumed
            key = functional.key;
            if (functional.keypad) modifiers |= Qt::KeypadModifier;
            if (functional.text) text = QString::fromUcs4(&functional.text, 1);
        } else if (codepoint < 32) {
            return true; // a C0 control has no key of its own; consumed
        } else {
            key = keyForCharacter(static_cast<char32_t>(codepoint));
            const int shifted = sequence.parameter(0, 1, 0);
            const char32_t character = static_cast<char32_t>(
                (modifiers & Qt::ShiftModifier) && shifted >= 32 && isUnicodeScalar(shifted)
                    ? shifted
                    : codepoint);
            text = QString::fromUcs4(&character, 1);
        }
    }
    // Text the terminal reports explicitly (flag 16) wins; otherwise a
    // Ctrl or Alt chord is a shortcut and must not insert its key as text,
    // matching the legacy path where Ctrl+A arrives as a bare control byte.
    if (sequence.params.size() == 3) {
        QString reported;
        for (int value : sequence.params[2]) {
            if (value < 32 || value == 127 || !isUnicodeScalar(value)) continue;
            const char32_t character = static_cast<char32_t>(value);
            reported += QString::fromUcs4(&character, 1);
        }
        if (!reported.isEmpty())
            text = reported;
        else if (shortcut)
            text.clear();
    } else if (shortcut) {
        text.clear();
    }
    if (m_keyCallback) m_keyCallback(key, text, modifiers);
    return true;
}

bool InputDecoder::hasPendingEscape() const {
    return !m_bracketedPaste && !m_discardKind && m_buffer == "\033";
}

void InputDecoder::flushPendingEscape() {
    if (!hasPendingEscape()) return;
    m_buffer.clear();
    if (m_keyCallback) m_keyCallback(Qt::Key_Escape, "", 0);
}

void InputDecoder::decode(const char* buffer, size_t size) {
    constexpr size_t MaxPasteBufferSize = 1024 * 1024;
    if (!buffer) return;
    // Process large reads incrementally so control framing is never lost by
    // truncating the beginning of an input read.
    while (size) {
        const size_t chunk = (std::min)(size, size_t(4096));
        m_buffer.append(buffer, chunk);
        buffer += chunk;
        size -= chunk;
        processBuffer();
        if (m_bracketedPaste && m_buffer.size() > MaxPasteBufferSize)
            m_buffer.erase(0, m_buffer.size() - MaxPasteBufferSize);
    }
}

void InputDecoder::processBuffer() {
    size_t i = 0;
    while (i < m_buffer.size()) {
        if (m_discardKind) {
            const unsigned char c = m_buffer[i];
            if (c == 0x18 || c == 0x1a || (m_discardKind == 3 && c == 7) ||
                (m_discardEscape && c == '\\')) {
                m_discardKind = 0;
                m_discardEscape = false;
                ++i;
            } else if (m_discardEscape) {
                // The previous ESC begins a fresh sequence, not payload.
                m_discardKind = 0;
                m_discardEscape = false;
                m_buffer.insert(i, 1, '\033');
            } else if (m_discardKind == 1 && c >= 0x40 && c <= 0x7e) {
                m_discardKind = 0;
                ++i;
            } else {
                m_discardEscape = c == 0x1b;
                ++i;
            }
            continue;
        }
        if (m_bracketedPaste) {
            constexpr std::string_view PasteEnd = "\033[201~";
            const size_t end = m_buffer.find(PasteEnd, i);
            if (end == std::string::npos) break;
            const QString text = QString::fromUtf8(m_buffer.data() + i, end - i);
            if (m_pasteCallback)
                m_pasteCallback(text);
            else if (m_keyCallback && !text.isEmpty())
                m_keyCallback(0, text, 0);
            i = end + PasteEnd.size();
            m_bracketedPaste = false;
            continue;
        }

        unsigned char c = m_buffer[i];

        if (c >= 1 && c <= 26 && c != 8 && c != 9 && c != 10 && c != 13) {
            int qtKey = c - 1 + Qt::Key_A;
            if (m_keyCallback) m_keyCallback(qtKey, "", Qt::ControlModifier);
            i++;
            continue;
        }

        // Check for escape sequences, preserving the existing standalone ESC
        // buffering contract until a following byte resolves it. SS3 has one
        // final byte by definition, and string controls are framed only when
        // they look like a reply; anything else is Escape followed by a key.
        if (c == '\033') {
            if (i + 1 == m_buffer.size()) break;
            const unsigned char next = m_buffer[i + 1];
            bool framed = next == '[';
            if (next == 'O' || next == '_' || next == ']' || next == 'P') {
                if (i + 2 == m_buffer.size()) break;
                const unsigned char first = m_buffer[i + 2];
                framed = next == 'O' ? isFinalByte(first) : looksLikeReply(next, first);
            }
            if (framed) {
                ControlSequence sequence;
                size_t consumed = 0;
                const auto bytes = std::string_view(m_buffer).substr(i);
                const auto result = parseSequence(bytes, sequence, consumed, m_stringScanOffset);
                if (result == ParseResult::Incomplete) {
                    // Resume string scanning near the previous end (an ESC may
                    // still need its following byte). Avoid quadratic large reads.
                    if (sequence.kind >= ControlSequence::Apc)
                        m_stringScanOffset = (std::max)(size_t(2), bytes.size() - 1);
                    break;
                }
                m_stringScanOffset = 2;
                if (result != ParseResult::Complete && bytes.rfind("\033]52;", 0) == 0 &&
                    m_clipboardErrorCallback)
                    m_clipboardErrorCallback();
                i += consumed;
                if (result == ParseResult::Complete)
                    dispatchSequence(sequence);
                else if (result == ParseResult::Oversized)
                    m_discardKind = sequence.kind == ControlSequence::Osc   ? 3
                                    : sequence.kind >= ControlSequence::Apc ? 2
                                                                            : 1;
                continue;
            }
            if (m_keyCallback) m_keyCallback(Qt::Key_Escape, "", 0);
            ++i;
            continue;
        }

        if (c == '\t') {
            if (m_keyCallback) m_keyCallback(Qt::Key_Tab, "", 0);
            i++;
            continue;
        }

        if (c == '\r' || c == '\n') {
            if (m_keyCallback) m_keyCallback(Qt::Key_Return, "\n", 0);
            i++;
            continue;
        }

        if (c == '\x7f' || c == '\x08') {
            if (m_keyCallback) m_keyCallback(Qt::Key_Backspace, "", 0);
            i++;
            continue;
        }

        if (c >= 1 && c <= 26) {
            const int key = Qt::Key_A + (c - 1);
            if (m_keyCallback) m_keyCallback(key, QString(), Qt::ControlModifier);
            i++;
            continue;
        }

        // Decode UTF-8 multi-byte characters
        if ((c & 0x80) != 0) {
            size_t len = 1;
            if ((c & 0xE0) == 0xC0)
                len = 2;
            else if ((c & 0xF0) == 0xE0)
                len = 3;
            else if ((c & 0xF8) == 0xF0)
                len = 4;

            if (i + len <= m_buffer.size()) {
                QString text = QString::fromUtf8(m_buffer.data() + i, len);
                const auto codepoints = text.toUcs4();
                int unicodeKey = codepoints.isEmpty() ? 0 : keyForCharacter(codepoints[0]);
                if (m_keyCallback) m_keyCallback(unicodeKey, text, 0);
                i += len;
                continue;
            } else {
                break; // Incomplete UTF-8
            }
        }

        if (c >= 32 && c < 127) {
            QString text = QString((char)c);
            if (m_keyCallback) m_keyCallback(keyForCharacter(c), text, 0);
            i++;
            continue;
        }

        i++; // Unknown or unhandled control character
    }

    // Erase processed bytes
    m_buffer.erase(0, i);
}

} // namespace qmltui
