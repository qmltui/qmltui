#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QSize>
#include <QtQml/qqmlregistration.h>
#include <Qamui/Core/primitives/QamuiExport.h>
#include <cstdint>

namespace qamui {

// Colour resolution used when encoding cells for the terminal.
enum class ColorDepth : uint8_t {
    Mono,        // no colour sequences at all
    Basic16,     // SGR 30-37 / 90-97
    Indexed256,  // SGR 38;5;n
    TrueColor    // SGR 38;2;r;g;b
};

// Environment-derived defaults, refined by output-session query replies.
// Override with QAMUI_COLOR_DEPTH (mono|16|256|truecolor) and QAMUI_SYNC_OUTPUT
// (0|1).
struct QAMUI_CORE_EXPORT TerminalCapabilities {
    Q_GADGET
    QML_VALUE_TYPE(terminalCapabilities)
    Q_PROPERTY(bool kittyKeyboard READ supportsKittyKeyboard)
    Q_PROPERTY(bool kittyGraphics READ supportsKittyGraphics)
    Q_PROPERTY(bool sixelGraphics READ supportsSixelGraphics)
    Q_PROPERTY(QSize cellPixelSize READ pixelSize)
    Q_PROPERTY(bool syncOutput READ supportsSyncOutput)
public:
    bool kittyKeyboard = false;
    bool kittyGraphics = false;
    bool sixelGraphics = false;
    QSize cellPixelSize;
    // The terminal is known to parse APC strings, so the kitty graphics probe
    // cannot be printed onto the screen. Set from TERM and TERM_PROGRAM; not a
    // capability of its own and not exposed to QML.
    bool mayProbeGraphics = false;
    bool supportsKittyKeyboard() const { return kittyKeyboard; }
    bool supportsKittyGraphics() const { return kittyGraphics; }
    bool supportsSixelGraphics() const { return sixelGraphics; }
    QSize pixelSize() const { return cellPixelSize; }
    bool supportsSyncOutput() const { return syncOutput; }
    bool operator==(const TerminalCapabilities&) const = default;

    ColorDepth colorDepth = ColorDepth::TrueColor;
    bool syncOutput = true;   // DEC private mode 2026 frame bracketing
    bool isTty = true;
    QString termProgram;

    static TerminalCapabilities detect(
        const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment(),
        bool stdoutIsTty = true);

    // The one place QAMUI_COLOR_DEPTH, QAMUI_SYNC_OUTPUT, QAMUI_KITTY_KEYBOARD and
    // QAMUI_KITTY_GRAPHICS are honoured. detect() applies it to the environment
    // defaults; the engine applies it again after terminal replies so an
    // override wins over what the terminal claims. 0, false and off disable.
    static void applyEnvironmentOverrides(
        TerminalCapabilities &capabilities,
        const QProcessEnvironment &environment = QProcessEnvironment::systemEnvironment());

    static QString colorDepthName(ColorDepth depth);
};

} // namespace qamui
