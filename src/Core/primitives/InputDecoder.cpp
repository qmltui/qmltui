#include "InputDecoder.h"
#include <QList>
#include <Qt>
#include <string_view>
#include <utility>
#include <limits>

namespace qtui {

InputDecoder::InputDecoder() {}

InputDecoder::~InputDecoder() {}

void InputDecoder::setKeyCallback(KeyCallback cb) {
    m_keyCallback = std::move(cb);
}

void InputDecoder::setMouseCallback(MouseCallback cb) {
    m_mouseCallback = std::move(cb);
}

void InputDecoder::decode(const char* buffer, size_t size) {
    constexpr size_t MaxEscapeBufferSize = 4096;
    constexpr size_t MaxPasteBufferSize = 1024 * 1024;
    if (!buffer || size == 0) return;
    if (size > MaxPasteBufferSize) {
        buffer += size - MaxPasteBufferSize;
        size = MaxPasteBufferSize;
    }
    if (m_buffer.size() > MaxPasteBufferSize - size)
        m_buffer.erase(0, m_buffer.size() - (MaxPasteBufferSize - size));
    m_buffer.append(buffer, size);
    processBuffer();
    const size_t retainedLimit = m_bracketedPaste
        ? MaxPasteBufferSize : MaxEscapeBufferSize;
    if (m_buffer.size() > retainedLimit)
        m_buffer.erase(0, m_buffer.size() - retainedLimit);
}

void InputDecoder::processBuffer() {
    size_t i = 0;
    while (i < m_buffer.size()) {
        if (m_bracketedPaste) {
            constexpr std::string_view PasteEnd = "\033[201~";
            const size_t end = m_buffer.find(PasteEnd, i);
            if (end == std::string::npos)
                break;
            if (m_keyCallback && end > i)
                m_keyCallback(0, QString::fromUtf8(m_buffer.data() + i, end - i), 0);
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

        // Check for escape sequences
        if (c == '\033') {
            if (i + 1 == m_buffer.size()) {
                break;
            }
            if (m_buffer[i + 1] == '[') {
                constexpr std::string_view PasteStart = "\033[200~";
                if (m_buffer.size() - i < PasteStart.size() &&
                    PasteStart.starts_with(std::string_view(m_buffer).substr(i)))
                    break;
                if (std::string_view(m_buffer).substr(i).starts_with(PasteStart)) {
                    i += PasteStart.size();
                    m_bracketedPaste = true;
                    continue;
                }
                size_t j = i + 2;
                if (j == m_buffer.size()) break; // Incomplete

                // Check for Mouse SGR protocol: \033[<cb;cx;cyM or m
                if (m_buffer[j] == '<') {
                    j++;
                    bool complete = false;
                    size_t temp = j;
                    while (temp < m_buffer.size()) {
                        if (m_buffer[temp] == 'M' || m_buffer[temp] == 'm') {
                            complete = true;
                            break;
                        }
                        temp++;
                    }
                    if (!complete) break; // Wait for the rest

                    int button = 0;
                    int cx = 0;
                    int cy = 0;
                    bool release = false;

                    constexpr int MaxMouseField = 1'000'000;
                    const auto parseField = [this, &j](int& value) {
                        const size_t begin = j;
                        value = 0;
                        while (j < m_buffer.size() && m_buffer[j] >= '0' && m_buffer[j] <= '9') {
                            const int digit = m_buffer[j] - '0';
                            if (value > (MaxMouseField - digit) / 10)
                                return false;
                            value = value * 10 + digit;
                            ++j;
                        }
                        return j > begin;
                    };

                    const bool valid = parseField(button)
                        && j < m_buffer.size() && m_buffer[j++] == ';'
                        && parseField(cx)
                        && j < m_buffer.size() && m_buffer[j++] == ';'
                        && parseField(cy)
                        && j < m_buffer.size() && (m_buffer[j] == 'M' || m_buffer[j] == 'm');
                    if (!valid) {
                        i = temp + 1;
                        continue;
                    }
                    release = m_buffer[j++] == 'm';

                    const bool motion = (button & 32) != 0;
                    if (motion)
                        button &= ~32;
                    if (m_mouseCallback)
                        m_mouseCallback(button, cx - 1, cy - 1, release, motion);
                    i = j;
                    continue;
                }

                // Check for Arrow keys or Shift-Tab
                if (j < m_buffer.size()) {
                    if (m_buffer[j] == 'A') { // Up
                        if (m_keyCallback) m_keyCallback(Qt::Key_Up, "", 0);
                        i = j + 1;
                        continue;
                    } else if (m_buffer[j] == 'B') { // Down
                        if (m_keyCallback) m_keyCallback(Qt::Key_Down, "", 0);
                        i = j + 1;
                        continue;
                    } else if (m_buffer[j] == 'C') { // Right
                        if (m_keyCallback) m_keyCallback(Qt::Key_Right, "", 0);
                        i = j + 1;
                        continue;
                    } else if (m_buffer[j] == 'D') { // Left
                        if (m_keyCallback) m_keyCallback(Qt::Key_Left, "", 0);
                        i = j + 1;
                        continue;
                    } else if (m_buffer[j] == 'Z') { // Shift-Tab
                        if (m_keyCallback) m_keyCallback(Qt::Key_Backtab, "", 0);
                        i = j + 1;
                        continue;
                    }
                    if ((m_buffer[j] >= 'A' && m_buffer[j] <= 'Z') || (m_buffer[j] >= 'a' && m_buffer[j] <= 'z') || m_buffer[j] == '~') {
                        i = j + 1;
                        continue;
                    }

                    size_t finalPosition = std::string::npos;
                    for (size_t k = j; k < m_buffer.size(); k++) {
                        const unsigned char byte = m_buffer[k];
                        if (byte >= 0x40 && byte <= 0x7E) {
                            finalPosition = k;
                            break;
                        }
                    }
                    if (finalPosition == std::string::npos) {
                        break;
                    }

                    const char finalByte = m_buffer[finalPosition];
                    const std::string parameters =
                        m_buffer.substr(j, finalPosition - j);
                    const bool isBacktab =
                        (finalByte == 'Z')
                        || (finalByte == 'u' && parameters == "9;2")
                        || (finalByte == '~' && parameters == "27;2;9");
                    if (isBacktab && m_keyCallback) {
                        m_keyCallback(Qt::Key_Backtab, "", Qt::ShiftModifier);
                    } else if (finalByte == '~' && m_keyCallback) {
                        if (parameters == "11") m_keyCallback(Qt::Key_F1, "", 0);
                        else if (parameters == "12") m_keyCallback(Qt::Key_F2, "", 0);
                        else if (parameters == "13") m_keyCallback(Qt::Key_F3, "", 0);
                        else if (parameters == "14") m_keyCallback(Qt::Key_F4, "", 0);
                    }
                    i = finalPosition + 1;
                    continue;
                }
            } else if (m_buffer[i + 1] == 'O') {
                size_t j = i + 2;
                if (j == m_buffer.size()) break; // Incomplete
                if (m_buffer[j] == 'P') { if (m_keyCallback) m_keyCallback(Qt::Key_F1, "", 0); i = j + 1; continue; }
                else if (m_buffer[j] == 'Q') { if (m_keyCallback) m_keyCallback(Qt::Key_F2, "", 0); i = j + 1; continue; }
                else if (m_buffer[j] == 'R') { if (m_keyCallback) m_keyCallback(Qt::Key_F3, "", 0); i = j + 1; continue; }
                else if (m_buffer[j] == 'S') { if (m_keyCallback) m_keyCallback(Qt::Key_F4, "", 0); i = j + 1; continue; }
                else { i = j + 1; continue; }
            } else {
                if (m_keyCallback) m_keyCallback(Qt::Key_Escape, "", 0);
                i++;
                continue;
            }
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
                int unicodeKey = codepoints.isEmpty() ? 0 : static_cast<int>(codepoints[0]);
                if (m_keyCallback) m_keyCallback(unicodeKey, text, 0);
                i += len;
                continue;
            } else {
                break; // Incomplete UTF-8
            }
        }

        if (c >= 32 && c < 127) {
            QString text = QString((char)c);
            int qtKey = c;
            if (c >= 'a' && c <= 'z') {
                qtKey = c - 'a' + Qt::Key_A;
            }
            if (m_keyCallback) m_keyCallback(qtKey, text, 0);
            i++;
            continue;
        }

        i++; // Unknown or unhandled control character
    }

    // Erase processed bytes
    m_buffer.erase(0, i);
}

} // namespace qtui
