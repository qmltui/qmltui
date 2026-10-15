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

/**
 * @brief Root visual item and window manager for QmlTui terminal applications.
 *
 * Terminal establishes the top-level screen container, tracks terminal size
 * and resize events (SIGWINCH / console buffer events), routes input events
 * to focused items, and interfaces with the underlying TerminalBackend.
 *
 * ### Modes
 * - **AltScreen**: Takes over the terminal buffer (alternate screen buffer smcup/rmcup),
 *   clears the screen, and manages full window redrawing.
 * - **Inline**: Emits stream output directly into the existing terminal scrollback
 *   below the shell prompt without clearing previous history.
 *
 * Applications should place central keyboard handling (such as Quit on Escape/Q)
 * on the root Terminal item.
 *
 * @thread_safety Main/engine thread only.
 * @see Item, TerminalEngine, TerminalBackend
 */
class QMLTUI_CORE_EXPORT Terminal : public Item, public QQmlParserStatus {
  Q_OBJECT
  QML_ELEMENT
  Q_INTERFACES(QQmlParserStatus)

  /// @brief Queryable host terminal capabilities (color depth, mouse reporting, Unicode).
  Q_PROPERTY(qmltui::TerminalCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
  /// @brief Window title set via VT OSC 2 / OSC 0 escape sequences.
  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  /// @brief Background color for the terminal window matrix. Defaults to "default".
  Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
  /// @brief Display mode: AltScreen (alternate screen) or Inline (scrollback stream).
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  /// @brief Clipboard synchronization mode (Disabled or Osc52 ANSI escape writes).
  Q_PROPERTY(ClipboardMode clipboardMode READ clipboardMode WRITE setClipboardMode NOTIFY clipboardModeChanged)
  /// @brief Whether mouse tracking escapes (SGR 1006 / 1000) are enabled on the host terminal.
  Q_PROPERTY(bool mouseReporting READ mouseReporting WRITE setMouseReporting NOTIFY mouseReportingChanged)
  /// @brief Currently hovered Item under the mouse cursor, or nullptr.
  Q_PROPERTY(qmltui::Item* hoveredItem READ hoveredItem NOTIFY hoveredItemChanged)
  /// @brief Total width of the host terminal screen in character columns.
  Q_PROPERTY(int terminalWidth READ terminalWidth NOTIFY terminalSizeChanged FINAL)
  /// @brief Total height of the host terminal screen in character rows.
  Q_PROPERTY(int terminalHeight READ terminalHeight NOTIFY terminalSizeChanged FINAL)
  /// @brief Whether the host terminal window currently has focus (?1004 focus reporting).
  Q_PROPERTY(bool applicationActive READ applicationActive NOTIFY applicationActiveChanged)
  /// @brief Progress value in [0.0, 1.0] for OSC 9;4 terminal progress reporting.
  Q_PROPERTY(qreal progress READ progress WRITE setProgress NOTIFY progressChanged)
  /// @brief Progress state for OSC 9;4 terminal progress reporting.
  Q_PROPERTY(ProgressState progressState READ progressState WRITE setProgressState NOTIFY progressStateChanged)

public:
  enum class Mode {
      AltScreen,
      Inline
  };
  Q_ENUM(Mode)
  enum class ClipboardMode {
      Disabled,
      Osc52
  };
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

  explicit Terminal(QObject *parent = nullptr);
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
  Popup *topModalPopup() const;
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

  void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;

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
  void paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                    const ClipRect &clipRect) const override;

  TerminalEngine *m_engine = nullptr;
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
