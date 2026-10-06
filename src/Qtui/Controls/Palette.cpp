#include "Palette.h"

#include <QCoreApplication>

namespace qtui {

Palette* Palette::instance()
{
    static Palette* palette = new Palette(QCoreApplication::instance());
    return palette;
}

Palette* Palette::create(QQmlEngine*, QJSEngine* jsEngine)
{
    Palette* palette = instance();
    // The engine must not delete the shared instance.
    QJSEngine::setObjectOwnership(palette, QJSEngine::CppOwnership);
    Q_UNUSED(jsEngine);
    return palette;
}

Palette::Palette(QObject* parent)
    : QObject(parent)
{
    applyPreset(Preset::Terminal);
}

void Palette::assign(Color& role, Color value)
{
    if (role == value)
        return;
    role = value;
    emit changed();
}

void Palette::applyPreset(Preset preset)
{
    switch (preset) {
    case Preset::Terminal:
        // ANSI names resolve to the user's terminal theme; the window stays
        // transparent so the terminal's own background shows through.
        m_window = Color::fromString(QStringLiteral("transparent"));
        m_windowText = Color::fromString(QStringLiteral("white"));
        m_base = Color::fromString(QStringLiteral("transparent"));
        m_text = Color::fromString(QStringLiteral("white"));
        m_button = Color::fromString(QStringLiteral("transparent"));
        m_buttonText = Color::fromString(QStringLiteral("white"));
        m_highlight = Color::fromString(QStringLiteral("yellow"));
        m_highlightedText = Color::fromString(QStringLiteral("black"));
        m_mid = Color::fromString(QStringLiteral("white"));
        m_placeholderText = Color::fromString(QStringLiteral("gray"));
        m_accent = Color::fromString(QStringLiteral("cyan"));
        break;
    case Preset::Dark:
        m_window = Color::fromString(QStringLiteral("#0d1117"));
        m_windowText = Color::fromString(QStringLiteral("#c9d1d9"));
        m_base = Color::fromString(QStringLiteral("#161b22"));
        m_text = Color::fromString(QStringLiteral("#c9d1d9"));
        m_button = Color::fromString(QStringLiteral("#21262d"));
        m_buttonText = Color::fromString(QStringLiteral("#c9d1d9"));
        m_highlight = Color::fromString(QStringLiteral("#2cde85"));
        m_highlightedText = Color::fromString(QStringLiteral("#0d1117"));
        m_mid = Color::fromString(QStringLiteral("#30363d"));
        m_placeholderText = Color::fromString(QStringLiteral("#8b949e"));
        m_accent = Color::fromString(QStringLiteral("#2cde85"));
        break;
    }
    emit changed();
}

} // namespace qtui
