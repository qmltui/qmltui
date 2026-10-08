#include "TerminalCapabilities.h"

namespace qtui {

namespace {

bool parseColorDepth(const QString &value, ColorDepth &depth)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QLatin1String("mono") || normalized == QLatin1String("none")
        || normalized == QLatin1String("0")) {
        depth = ColorDepth::Mono;
    } else if (normalized == QLatin1String("16") || normalized == QLatin1String("basic")
               || normalized == QLatin1String("ansi")) {
        depth = ColorDepth::Basic16;
    } else if (normalized == QLatin1String("256") || normalized == QLatin1String("indexed")) {
        depth = ColorDepth::Indexed256;
    } else if (normalized == QLatin1String("truecolor") || normalized == QLatin1String("24bit")
               || normalized == QLatin1String("rgb")) {
        depth = ColorDepth::TrueColor;
    } else {
        return false;
    }
    return true;
}

bool flagEnabled(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    return !normalized.isEmpty() && normalized != QLatin1String("0")
        && normalized != QLatin1String("false") && normalized != QLatin1String("off");
}

} // namespace

TerminalCapabilities TerminalCapabilities::detect(const QProcessEnvironment &environment,
                                                  bool stdoutIsTty)
{
    TerminalCapabilities capabilities;
    capabilities.isTty = stdoutIsTty;
    capabilities.termProgram = environment.value(QStringLiteral("TERM_PROGRAM"));

    const QString term = environment.value(QStringLiteral("TERM")).trimmed().toLower();
    const QString colorTerm = environment.value(QStringLiteral("COLORTERM")).trimmed().toLower();
    const QString program = capabilities.termProgram.trimmed().toLower();

    // Colour depth, first match wins; QTUI_COLOR_DEPTH is applied last as an override.
    if (!environment.value(QStringLiteral("NO_COLOR")).isEmpty()) {
        capabilities.colorDepth = ColorDepth::Mono;
    } else if (term == QLatin1String("dumb")) {
        capabilities.colorDepth = ColorDepth::Mono;
    } else if (colorTerm == QLatin1String("truecolor") || colorTerm == QLatin1String("24bit")
               || term.endsWith(QLatin1String("-direct"))) {
        capabilities.colorDepth = ColorDepth::TrueColor;
    } else if (program == QLatin1String("iterm.app") || program == QLatin1String("wezterm")
               || program == QLatin1String("vscode") || program == QLatin1String("ghostty")
               || program == QLatin1String("kitty") || program == QLatin1String("hyper")) {
        capabilities.colorDepth = ColorDepth::TrueColor;
    } else if (program == QLatin1String("apple_terminal")) {
        capabilities.colorDepth = ColorDepth::Indexed256;
    } else if (environment.contains(QStringLiteral("WT_SESSION"))
               || environment.contains(QStringLiteral("WT_PROFILE_ID"))
               || environment.contains(QStringLiteral("WSL_INTEROP"))
               || environment.contains(QStringLiteral("WSL_DISTRO_NAME"))) {
        // Windows Terminal, natively or through WSL, supports 24-bit colour even
        // when TERM only advertises 256 (the usual WSL default). Without this,
        // desaturated true colours quantise to the 256-cube grey ramp and warm
        // tones render as grey. Legacy conhost is rare now; QTUI_COLOR_DEPTH
        // overrides if needed.
        capabilities.colorDepth = ColorDepth::TrueColor;
    } else if (term.contains(QLatin1String("256color"))) {
        capabilities.colorDepth = ColorDepth::Indexed256;
    } else {
        capabilities.colorDepth = ColorDepth::TrueColor;   // previous unconditional behaviour
    }

    // Synchronized output: unknown private modes are ignored by terminals, so
    // it is on unless the output is not a terminal at all.
    capabilities.syncOutput = stdoutIsTty && term != QLatin1String("dumb");

    // Terminals that implement the kitty graphics protocol, and therefore parse
    // APC strings instead of printing them. Konsole and kitty do not set
    // TERM_PROGRAM, so their own variables are checked as well.
    capabilities.mayProbeGraphics = program == QLatin1String("kitty")
        || program == QLatin1String("ghostty") || program == QLatin1String("wezterm")
        || term == QLatin1String("xterm-kitty") || term == QLatin1String("xterm-ghostty")
        || environment.contains(QStringLiteral("KITTY_WINDOW_ID"))
        || environment.contains(QStringLiteral("KONSOLE_VERSION"))
        || environment.contains(QStringLiteral("WEZTERM_EXECUTABLE"))
        || environment.contains(QStringLiteral("GHOSTTY_RESOURCES_DIR"));

    applyEnvironmentOverrides(capabilities, environment);
    return capabilities;
}

void TerminalCapabilities::applyEnvironmentOverrides(TerminalCapabilities &capabilities,
                                                     const QProcessEnvironment &environment)
{
    ColorDepth requested = ColorDepth::TrueColor;
    if (parseColorDepth(environment.value(QStringLiteral("QTUI_COLOR_DEPTH")), requested))
        capabilities.colorDepth = requested;
    if (environment.contains(QStringLiteral("QTUI_SYNC_OUTPUT")))
        capabilities.syncOutput = flagEnabled(environment.value(QStringLiteral("QTUI_SYNC_OUTPUT")));
    if (environment.contains(QStringLiteral("QTUI_KITTY_KEYBOARD"))
        && !flagEnabled(environment.value(QStringLiteral("QTUI_KITTY_KEYBOARD"))))
        capabilities.kittyKeyboard = false;
    if (environment.contains(QStringLiteral("QTUI_KITTY_GRAPHICS"))
        && !flagEnabled(environment.value(QStringLiteral("QTUI_KITTY_GRAPHICS"))))
        capabilities.kittyGraphics = false;
}

QString TerminalCapabilities::colorDepthName(ColorDepth depth)
{
    switch (depth) {
    case ColorDepth::Mono: return QStringLiteral("mono");
    case ColorDepth::Basic16: return QStringLiteral("16");
    case ColorDepth::Indexed256: return QStringLiteral("256");
    case ColorDepth::TrueColor: return QStringLiteral("truecolor");
    }
    return QStringLiteral("truecolor");
}

} // namespace qtui
