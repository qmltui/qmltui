#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <cstdint>

namespace qtui {

// Colour resolution used when encoding cells for the terminal.
enum class ColorDepth : uint8_t {
    Mono,        // no colour sequences at all
    Basic16,     // SGR 30-37 / 90-97
    Indexed256,  // SGR 38;5;n
    TrueColor    // SGR 38;2;r;g;b
};

// What the terminal is believed to support, derived once from the environment.
// Override with QTUI_COLOR_DEPTH (mono|16|256|truecolor) and QTUI_SYNC_OUTPUT
// (0|1).
struct TerminalCapabilities {
    ColorDepth colorDepth = ColorDepth::TrueColor;
    bool syncOutput = true;   // DEC private mode 2026 frame bracketing
    bool isTty = true;
    QString termProgram;

    static TerminalCapabilities detect(
        const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment(),
        bool stdoutIsTty = true);

    static QString colorDepthName(ColorDepth depth);
};

} // namespace qtui
