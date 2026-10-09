#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <Qtui/Core/primitives/TerminalCapabilities.h>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlParserStatus>

namespace qtui {

class TerminalEngine;
class Popup;

class QTUI_CORE_EXPORT Terminal : public Item, public QQmlParserStatus {
  Q_OBJECT
  QML_ELEMENT
  Q_INTERFACES(QQmlParserStatus)

  Q_PROPERTY(qtui::TerminalCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
  Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
  Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
  Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
  Q_PROPERTY(ClipboardMode clipboardMode READ clipboardMode WRITE setClipboardMode NOTIFY clipboardModeChanged)
  Q_PROPERTY(bool mouseReporting READ mouseReporting WRITE setMouseReporting NOTIFY mouseReportingChanged)
  Q_PROPERTY(qtui::Item* hoveredItem READ hoveredItem NOTIFY hoveredItemChanged)
  Q_PROPERTY(int terminalWidth READ terminalWidth NOTIFY terminalSizeChanged FINAL)
  Q_PROPERTY(int terminalHeight READ terminalHeight NOTIFY terminalSizeChanged FINAL)

public:
  enum class Mode {
      FullScreen,
      Inline
  };
  Q_ENUM(Mode)
  enum class ClipboardMode {
      Disabled,
      Osc52
  };
  Q_ENUM(ClipboardMode)

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
  Mode m_mode = Mode::FullScreen;
  ClipboardMode m_clipboardMode = ClipboardMode::Osc52;
  bool m_mouseReporting = true;
  bool m_mouseReportingExplicit = false;
};

} // namespace qtui
