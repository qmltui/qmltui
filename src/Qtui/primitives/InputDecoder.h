#pragma once

#include <QString>
#include <Qt>
#include <functional>
#include <string>

namespace qtui {

class InputDecoder {
public:
    using KeyCallback = std::function<void(int qtKey, const QString& text, int modifiers)>;
    using PasteCallback = std::function<void(const QString& text)>;
    using MouseCallback = std::function<void(int button, int x, int y, bool release, bool motion)>;
    struct MouseInput {
        int button = 3;
        int x = 0;
        int y = 0;
        bool release = false;
        bool motion = false;
        int modifiers = 0;
        int wheelDeltaX = 0;
        int wheelDeltaY = 0;
    };
    using DetailedMouseCallback = std::function<void(const MouseInput& input)>;

    InputDecoder();
    ~InputDecoder();

    void setKeyCallback(KeyCallback cb);
    void setPasteCallback(PasteCallback cb);
    void setMouseCallback(MouseCallback cb);
    void setDetailedMouseCallback(DetailedMouseCallback cb);

    void decode(const char* buffer, size_t size);

    /// The Qt key code a keypress producing \a codepoint carries. Letters map to
    /// Qt::Key_A..Qt::Key_Z whatever their case; every other character carries
    /// its code point, which matches Qt::Key_0..Key_9 and ASCII punctuation.
    /// Inline so Qtui::TestRunner can synthesize the same events as raw input.
    static constexpr int keyForCharacter(char32_t codepoint)
    {
        if (codepoint >= U'a' && codepoint <= U'z')
            return static_cast<int>(Qt::Key_A) + static_cast<int>(codepoint - U'a');
        return static_cast<int>(codepoint);
    }

private:
    void processBuffer();

    KeyCallback m_keyCallback;
    PasteCallback m_pasteCallback;
    MouseCallback m_mouseCallback;
    DetailedMouseCallback m_detailedMouseCallback;

    std::string m_buffer;
    bool m_bracketedPaste = false;
};

} // namespace qtui
