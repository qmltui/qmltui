#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/TerminalCapabilities.h>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlParserStatus>

namespace qmltui {

class TerminalEngine;
class Clipboard;
class Popup;

// Root visual item and session coordinator for a QmlTui application.
//
// Owns the TerminalEngine, tracks terminal dimensions, manages active keyboard
// focus, hover state, and Popup / InputScope overlays, and selects between
// full-viewport alternate-screen rendering (Mode::AltScreen) and inline
// scrollback stream rendering (Mode::Inline). Does not bind Q or Escape to
// quit automatically; applications define their own quit path.
class QMLTUI_CORE_EXPORT Terminal : public Item, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(
        qmltui::TerminalCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(ClipboardMode clipboardMode READ clipboardMode WRITE setClipboardMode NOTIFY
                   clipboardModeChanged)
    // Enables SGR mouse tracking (?1000h/?1002h/?1006h); any-motion (?1003h) is
    // added automatically while at least one item requests hover events.
    Q_PROPERTY(bool mouseReporting READ mouseReporting WRITE setMouseReporting NOTIFY
                   mouseReportingChanged)
    Q_PROPERTY(qmltui::Item* hoveredItem READ hoveredItem NOTIFY hoveredItemChanged)
    Q_PROPERTY(int terminalWidth READ terminalWidth NOTIFY terminalSizeChanged FINAL)
    Q_PROPERTY(int terminalHeight READ terminalHeight NOTIFY terminalSizeChanged FINAL)
    // Host terminal window focus state reported via DEC mode 1004 (?1004h).
    Q_PROPERTY(bool applicationActive READ applicationActive NOTIFY applicationActiveChanged)
    // Progress value in [0.0, 1.0] and state for OSC 9;4 terminal progress reporting.
    Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(ProgressState progressState READ progressState WRITE setProgressState NOTIFY
                   progressStateChanged)

public:
    enum class Mode { AltScreen, Inline };
    Q_ENUM(Mode)
    enum class ClipboardMode { Disabled, Osc52 };
    Q_ENUM(ClipboardMode)
    enum ProgressState {
        ProgressNone = 0,
        ProgressNormal = 1,
        ProgressError = 2,
        ProgressIndeterminate = 3,
        ProgressWarning = 4
    };
    Q_ENUM(ProgressState)

    using RunnerHook = void (*)(QQmlEngine*, Terminal*);
    static void registerRunnerHook(RunnerHook hook);

    explicit Terminal(QObject* parent = nullptr);
    ~Terminal() override;

    void classBegin() override {}
    void componentComplete() override;

    TerminalEngine* engine() const;
    TerminalCapabilities capabilities() const;
    // Framework resource commands only: preserve cursor and rendition state.
    bool enqueueControlSequence(QByteArray bytes);
    QByteArray takeControlSequences();
    bool enqueuePostControlSequence(QByteArray bytes);
    QByteArray takePostControlSequences();

    Q_INVOKABLE void copyToClipboard(const QString& text);
    Q_INVOKABLE void bell();

    bool applicationActive() const { return m_applicationActive; }
    void setApplicationActive(bool active);

    qreal progress() const { return m_progress; }
    void setProgress(qreal progress);

    ProgressState progressState() const { return m_progressState; }
    void setProgressState(ProgressState state);

    QString title() const { return m_title; }
    void setTitle(const QString& title);

    Color color() const { return m_color; }
    void setColor(Color color);

    void registerPopup(Popup* popup);
    void unregisterPopup(Popup* popup);
    const QList<Popup*>& activePopups() const { return m_popups; }
    QList<Popup*> orderedPopups() const;
    Popup* topModalPopup() const;
    QList<class InputScope*> orderedInputScopes() const;
    class InputScope* topModalScope() const;

    Mode mode() const { return m_mode; }
    void setMode(Mode mode);
    ClipboardMode clipboardMode() const { return m_clipboardMode; }
    void setClipboardMode(ClipboardMode mode);

    bool mouseReporting() const { return m_mouseReporting; }
    void setMouseReporting(bool enable);

    Item* focusedItem() const { return m_focusedItem.data(); }
    void setFocusedItem(Item* item);
    Item* hoveredItem() const { return m_hoveredItem.data(); }
    void setHoveredItem(Item* item);

    void setMouseMotionRequested(QObject* requester, bool requested);
    bool isMouseMotionRequester(const QObject* requester) const {
        return requester && m_mouseMotionRequesters.contains(const_cast<QObject*>(requester));
    }
    bool hasMouseMotionRequests() const { return !m_mouseMotionRequesters.isEmpty(); }

    void findFocusableItems(Item* root, QList<Item*>& list);

    int width() const override;
    void setWidth(int width) override;

    int height() const override;
    void setHeight(int height) override;

    int implicitHeight() const override;

    int terminalWidth() const;
    int terminalHeight() const;

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

signals:
    void capabilitiesChanged();
    void titleChanged();
    void colorChanged();
    void modeChanged();
    void clipboardModeChanged();
    void mouseReportingChanged();
    void clipboardCopyRequested(const QString& text);
    void focusedItemChanged();
    void hoveredItemChanged();
    void terminalSizeChanged();
    void mouseMotionRequestedChanged(bool requested);
    void applicationActiveChanged();
    void progressChanged();
    void progressStateChanged();

private:
    void initializeEngine();
    void paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                      const ClipRect& clipRect) const override;

    TerminalEngine* m_engine = nullptr;
    QPointer<Item> m_focusedItem;
    QPointer<Item> m_hoveredItem;
    QHash<QObject*, QMetaObject::Connection> m_mouseMotionRequesters;
    QList<Popup*> m_popups;
    quint64 m_popupSerial = 0;
    QString m_title;
    Color m_color = "transparent";
    uint32_t m_background = 0;
    Mode m_mode = Mode::AltScreen;
    QPointer<Clipboard> m_clipboard;
    ClipboardMode m_clipboardMode = ClipboardMode::Osc52;
    bool m_mouseReporting = true;
    bool m_mouseReportingExplicit = false;
    bool m_applicationActive = true;
    qreal m_progress = 0.0;
    ProgressState m_progressState = ProgressNone;
};

} // namespace qmltui
