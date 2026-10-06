#pragma once

#include <QtuiExport.h>
#include <Color.h>

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>

namespace qtui {

// Named color roles shared by every control. QML reads it as the
// `Palette` singleton from Qtui.Controls; C++ controls use instance().
//
// Roles follow QPalette so the vocabulary is familiar. The default preset,
// Terminal, uses ANSI named colors and a transparent window, so an application
// takes on whatever theme the user's terminal has. Dark is the Qtui brand look.
class QTUI_CONTROLS_EXPORT Palette : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(qtui::Color window READ window WRITE setWindow NOTIFY changed)
    Q_PROPERTY(qtui::Color windowText READ windowText WRITE setWindowText NOTIFY changed)
    Q_PROPERTY(qtui::Color base READ base WRITE setBase NOTIFY changed)
    Q_PROPERTY(qtui::Color text READ text WRITE setText NOTIFY changed)
    Q_PROPERTY(qtui::Color button READ button WRITE setButton NOTIFY changed)
    Q_PROPERTY(qtui::Color buttonText READ buttonText WRITE setButtonText NOTIFY changed)
    Q_PROPERTY(qtui::Color highlight READ highlight WRITE setHighlight NOTIFY changed)
    Q_PROPERTY(qtui::Color highlightedText READ highlightedText WRITE setHighlightedText NOTIFY changed)
    Q_PROPERTY(qtui::Color mid READ mid WRITE setMid NOTIFY changed)
    Q_PROPERTY(qtui::Color placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY changed)
    Q_PROPERTY(qtui::Color accent READ accent WRITE setAccent NOTIFY changed)

public:
    enum class Preset { Terminal, Dark };
    Q_ENUM(Preset)

    // The process-wide instance, shared by QML and C++.
    static Palette* instance();
    static Palette* create(QQmlEngine*, QJSEngine*);

    Color window() const { return m_window; }
    Color windowText() const { return m_windowText; }
    Color base() const { return m_base; }
    Color text() const { return m_text; }
    Color button() const { return m_button; }
    Color buttonText() const { return m_buttonText; }
    Color highlight() const { return m_highlight; }
    Color highlightedText() const { return m_highlightedText; }
    Color mid() const { return m_mid; }
    Color placeholderText() const { return m_placeholderText; }
    Color accent() const { return m_accent; }

    void setWindow(Color color) { assign(m_window, color); }
    void setWindowText(Color color) { assign(m_windowText, color); }
    void setBase(Color color) { assign(m_base, color); }
    void setText(Color color) { assign(m_text, color); }
    void setButton(Color color) { assign(m_button, color); }
    void setButtonText(Color color) { assign(m_buttonText, color); }
    void setHighlight(Color color) { assign(m_highlight, color); }
    void setHighlightedText(Color color) { assign(m_highlightedText, color); }
    void setMid(Color color) { assign(m_mid, color); }
    void setPlaceholderText(Color color) { assign(m_placeholderText, color); }
    void setAccent(Color color) { assign(m_accent, color); }

    // Replaces every role with the preset's values and emits changed() once.
    Q_INVOKABLE void applyPreset(qtui::Palette::Preset preset);

signals:
    // One signal for all roles: a theme switch changes most of them at once.
    void changed();

private:
    explicit Palette(QObject* parent = nullptr);
    void assign(Color& role, Color value);

    Color m_window, m_windowText, m_base, m_text, m_button, m_buttonText;
    Color m_highlight, m_highlightedText, m_mid, m_placeholderText, m_accent;
};

} // namespace qtui
