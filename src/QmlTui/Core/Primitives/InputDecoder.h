#pragma once

// Internal: streaming parser for terminal VT input bytes, mouse events, pastes, and query replies.

#include <QString>
#include <QByteArrayView>
#include <QVarLengthArray>
#include <Qt>
#include <cstdint>
#include <functional>
#include <string>

namespace qmltui {

// Streaming parser that turns raw terminal input bytes into key events, SGR
// and legacy mouse events, bracketed pastes, focus notifications, and framed
// terminal query replies (CSI, SS3, OSC, DCS, APC). Buffers incomplete UTF-8
// code points and multi-byte escape sequences across decode() calls.
class InputDecoder {
public:
    using KeyCallback = std::function<void(int qtKey, const QString& text, int modifiers)>;
    using PasteCallback = std::function<void(const QString& text)>;
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
    using FocusCallback = std::function<void(bool focused)>;

    struct ControlSequence {
        enum Kind : uint8_t {
            Csi,
            Ss3,
            Apc,
            Osc,
            Dcs,
        } kind = Csi;
        char privateMarker = 0;
        QVarLengthArray<QVarLengthArray<int, 3>, 8> params;
        QByteArrayView intermediates;
        char finalByte = 0;
        QByteArrayView payload;

        int parameter(qsizetype index, qsizetype sub = 0, int fallback = 0) const {
            return index >= 0 && sub >= 0 && index < params.size() && sub < params[index].size() &&
                           params[index][sub] >= 0
                       ? params[index][sub]
                       : fallback;
        }
    };
    // Views are valid only during this synchronous callback. Unhandled framed
    // controls are delivered here, never reinterpreted as keyboard text.
    // APC, DCS and OSC strings are framed only when their first payload byte
    // matches a known reply shape (kitty graphics 'G', DCS report markers, an
    // OSC number); otherwise ESC plus ']', '_' or 'P' is an Alt-prefixed key.
    using ReplyCallback = std::function<void(const ControlSequence&)>;
    void setReplyCallback(ReplyCallback cb);
    static constexpr qsizetype MaxClipboardTextBytes = 1024 * 1024;
    // Malformed/oversized OSC 52 frames are discarded, never keyboard input.
    void setClipboardErrorCallback(std::function<void()> cb);

    InputDecoder();
    ~InputDecoder();

    void setKeyCallback(KeyCallback cb);
    void setPasteCallback(PasteCallback cb);
    void setDetailedMouseCallback(DetailedMouseCallback cb);
    void setFocusCallback(FocusCallback cb);

    void decode(const char* buffer, size_t size);

    // A lone ESC is held back because it may begin a sequence whose remaining
    // bytes are still in flight. The owner runs a short timer after each read
    // and, when it expires with the ESC still alone, delivers it as Escape.
    // Terminals with the kitty disambiguate flag pushed never need this: they
    // report Escape as CSI 27 u.
    bool hasPendingEscape() const;
    void flushPendingEscape();

    // The Qt key code a keypress producing codepoint carries. Letters map to
    // Qt::Key_A..Qt::Key_Z whatever their case; every other character carries
    // its code point, which matches Qt::Key_0..Key_9 and ASCII punctuation.
    // Inline so qmltui::runner can synthesize the same events as raw input.
    static constexpr int keyForCharacter(char32_t codepoint) {
        if (codepoint >= U'a' && codepoint <= U'z')
            return static_cast<int>(Qt::Key_A) + static_cast<int>(codepoint - U'a');
        return static_cast<int>(codepoint);
    }

private:
    void processBuffer();
    void dispatchSequence(const ControlSequence& sequence);
    bool dispatchKittyKey(const ControlSequence& sequence);

    ReplyCallback m_replyCallback;
    std::function<void()> m_clipboardErrorCallback;
    size_t m_stringScanOffset = 2;
    int m_discardKind = 0; // 1: CSI/SS3, 2: APC/DCS, 3: OSC
    bool m_discardEscape = false;
    KeyCallback m_keyCallback;
    PasteCallback m_pasteCallback;
    DetailedMouseCallback m_detailedMouseCallback;
    FocusCallback m_focusCallback;

    std::string m_buffer;
    bool m_bracketedPaste = false;
};

} // namespace qmltui
