#pragma once

#include <QString>
#include <functional>
#include <string>

namespace qtui {

class InputDecoder {
public:
    using KeyCallback = std::function<void(int qtKey, const QString& text, int modifiers)>;
    using MouseCallback = std::function<void(int button, int x, int y, bool release, bool motion)>;

    InputDecoder();
    ~InputDecoder();

    void setKeyCallback(KeyCallback cb);
    void setMouseCallback(MouseCallback cb);

    void decode(const char* buffer, size_t size);

private:
    void processBuffer();

    KeyCallback m_keyCallback;
    MouseCallback m_mouseCallback;

    std::string m_buffer;
    bool m_bracketedPaste = false;
};

} // namespace qtui
