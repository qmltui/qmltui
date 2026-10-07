#include "InputDecoder.h"
#include <QList>
#include <Qt>
#include <string_view>
#include <utility>
#include <limits>

namespace qtui {

namespace {

using Sequence = InputDecoder::ControlSequence;

enum class ParseResult { Complete, Incomplete, Invalid, Oversized };

bool isFinalByte(unsigned char c);

ParseResult parseSequence(std::string_view bytes, Sequence& sequence, size_t& consumed) {
    const char introducer = bytes[1];
    sequence.kind = introducer == '[' ? Sequence::Csi : introducer == 'O' ? Sequence::Ss3
        : introducer == '_' ? Sequence::Apc : introducer == ']' ? Sequence::Osc : Sequence::Dcs;
    const bool string = sequence.kind >= Sequence::Apc;
    const size_t limit = string ? 4096 : 64;
    bool intermediates = false;
    size_t intermediateStart = 0;
    for (size_t pos = 2; pos < bytes.size(); ++pos) {
        const unsigned char c = bytes[pos];
        consumed = pos + 1;
        if (c == 0x18 || c == 0x1a)
            return ParseResult::Invalid;
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
                sequence.intermediates = QByteArrayView(bytes.data() + intermediateStart, pos - intermediateStart);
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
        if (c == ';') sequence.params.append({-1});
        else if (c == ':') sequence.params.last().append(-1);
        else if (c >= '0' && c <= '9') {
            int& value = sequence.params.last().last();
            if (value < 0) value = 0;
            if (value > (std::numeric_limits<int>::max() - (c - '0')) / 10)
                return ParseResult::Oversized;
            value = value * 10 + c - '0';
        } else return ParseResult::Oversized;
    }
    return ParseResult::Incomplete;
}

bool isFinalByte(unsigned char c) {
    return c >= 0x40 && c <= 0x7e;
}

// A string control (APC, DCS, OSC) is framed only when its first payload byte
// matches a reply Qtui or a terminal query can produce. Otherwise ESC followed
// by ']', '_' or 'P' is an Alt-prefixed keystroke from a metaSendsEscape
// terminal and must never swallow the typing that follows it.
bool looksLikeReply(unsigned char introducer, unsigned char first) {
    switch (introducer) {
    case '_': return first == 'G';                      // kitty graphics
    case 'P': return first == '$' || first == '>'       // DECRPSS, XTVERSION
                  || first == '!' || first == '+';      // DECRPTUI, XTGETTCAP
    case ']': return first >= '0' && first <= '9';      // OSC colour and clipboard replies
    default: return false;
    }
}

int xtermModifiers(const Sequence& sequence) {
    const int value = sequence.parameter(1, 0, 1);
    if (value < 1 || value > 8) return 0;
    const int bits = value - 1;
    int modifiers = 0;
    if (bits & 1) modifiers |= Qt::ShiftModifier;
    if (bits & 2) modifiers |= Qt::AltModifier;
    if (bits & 4) modifiers |= Qt::ControlModifier;
    return modifiers;
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

void InputDecoder::setReplyCallback(ReplyCallback cb) {
    m_replyCallback = std::move(cb);
}

void InputDecoder::dispatchSequence(const ControlSequence& sequence) {
    const char final = sequence.finalByte;
    if (sequence.kind == Sequence::Csi && sequence.privateMarker == '<'
        && sequence.intermediates.isEmpty() && (final == 'M' || final == 'm')) {
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
    if (!sequence.privateMarker && sequence.intermediates.isEmpty()
        && (sequence.kind == Sequence::Csi || sequence.kind == Sequence::Ss3)) {
        const int first = sequence.parameter(0);
        if (sequence.kind == Sequence::Csi && final == '~' && first == 200) {
            m_bracketedPaste = true;
            return;
        }
        int key = 0;
        int modifiers = xtermModifiers(sequence);
        if (final == 'Z' || (final == 'u' && first == 9 && sequence.parameter(1) == 2)
            || (final == '~' && first == 27 && sequence.parameter(1) == 2
                && sequence.parameter(2) == 9)) {
            key = Qt::Key_Backtab;
            modifiers = Qt::ShiftModifier;
        } else if (final == '~') {
            static constexpr std::pair<int, int> keys[] = {
                {1, Qt::Key_Home}, {2, Qt::Key_Insert}, {3, Qt::Key_Delete},
                {4, Qt::Key_End}, {5, Qt::Key_PageUp}, {6, Qt::Key_PageDown},
                {7, Qt::Key_Home}, {8, Qt::Key_End},
                {11, Qt::Key_F1}, {12, Qt::Key_F2}, {13, Qt::Key_F3}, {14, Qt::Key_F4},
                {15, Qt::Key_F5}, {17, Qt::Key_F6}, {18, Qt::Key_F7}, {19, Qt::Key_F8},
                {20, Qt::Key_F9}, {21, Qt::Key_F10}, {23, Qt::Key_F11}, {24, Qt::Key_F12}
            };
            for (const auto& [parameter, value] : keys)
                if (first == parameter) { key = value; break; }
        } else {
            static constexpr std::pair<char, int> keys[] = {
                {'A', Qt::Key_Up}, {'B', Qt::Key_Down}, {'C', Qt::Key_Right},
                {'D', Qt::Key_Left}, {'H', Qt::Key_Home}, {'F', Qt::Key_End},
                {'P', Qt::Key_F1}, {'Q', Qt::Key_F2}, {'R', Qt::Key_F3}, {'S', Qt::Key_F4}
            };
            for (const auto& [character, value] : keys)
                if (final == character) { key = value; break; }
        }
        if (key) {
            if (m_keyCallback) m_keyCallback(key, QString(), modifiers);
            return;
        }
    }
    if (m_replyCallback) m_replyCallback(sequence);
}

void InputDecoder::decode(const char* buffer, size_t size) {
    constexpr size_t MaxPasteBufferSize = 1024 * 1024;
    if (!buffer) return;
    // Process large reads incrementally so control framing is never lost by
    // truncating the beginning of an input read.
    while (size) {
        const size_t chunk = std::min(size, size_t(4096));
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
            if (c == 0x18 || c == 0x1a || (m_discardKind == 3 && c == 7)
                || (m_discardEscape && c == '\\')) {
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
            if (end == std::string::npos)
                break;
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
                const auto result = parseSequence(std::string_view(m_buffer).substr(i), sequence, consumed);
                if (result == ParseResult::Incomplete) break;
                i += consumed;
                if (result == ParseResult::Complete) dispatchSequence(sequence);
                else if (result == ParseResult::Oversized)
                    m_discardKind = sequence.kind == ControlSequence::Osc ? 3
                        : sequence.kind >= ControlSequence::Apc ? 2 : 1;
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
            if (m_keyCallback)
                m_keyCallback(key, QString(), Qt::ControlModifier);
            i++;
            continue;
        }

        // Decode UTF-8 multi-byte characters
        if ((c & 0x80) != 0) {
            size_t len = 1;
            if ((c & 0xE0) == 0xC0) len = 2;
            else if ((c & 0xF0) == 0xE0) len = 3;
            else if ((c & 0xF8) == 0xF0) len = 4;
            
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

} // namespace qtui
