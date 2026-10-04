#pragma once

#include "Rectangle.h"
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlParserStatus>

namespace qtui {

class TerminalEngine;
class Popup;

class Terminal : public Rectangle, public QQmlParserStatus {
  Q_OBJECT
  QML_ELEMENT
  Q_INTERFACES(QQmlParserStatus)

  Q_PROPERTY(bool useAlternateScreen READ useAlternateScreen WRITE setUseAlternateScreen NOTIFY useAlternateScreenChanged)
  Q_PROPERTY(bool fillTerminal READ fillTerminal WRITE setFillTerminal NOTIFY fillTerminalChanged)
  Q_PROPERTY(RendererMode rendererMode READ rendererMode WRITE setRendererMode NOTIFY rendererModeChanged)
  Q_PROPERTY(ClipboardMode clipboardMode READ clipboardMode WRITE setClipboardMode NOTIFY clipboardModeChanged)
  Q_PROPERTY(bool mouseReporting READ mouseReporting WRITE setMouseReporting NOTIFY mouseReportingChanged)
  Q_PROPERTY(qtui::Item* hoveredItem READ hoveredItem NOTIFY hoveredItemChanged)

public:
  enum class RendererMode {
      Frame,
      Stream
  };
  Q_ENUM(RendererMode)
  enum class ClipboardMode {
      Disabled,
      Osc52
  };
  Q_ENUM(ClipboardMode)

  using TestRunnerHook = void (*)(QQmlEngine*, Terminal*);
  static void registerTestRunnerHook(TestRunnerHook hook);

  explicit Terminal(QObject *parent = nullptr);
  ~Terminal() override;

  void classBegin() override {}
  void componentComplete() override;

  TerminalEngine* engine() const { return m_engine; }

  Q_INVOKABLE void print(const QString& text);
  Q_INVOKABLE void append(qtui::Item* item);
  Q_INVOKABLE void copyToClipboard(const QString& text);

  bool useAlternateScreen() const { return m_useAlternateScreen; }
  void setUseAlternateScreen(bool use);

  void registerPopup(Popup* popup);
  void unregisterPopup(Popup* popup);
  const QList<Popup*>& activePopups() const { return m_popups; }
  QList<Popup*> orderedPopups() const;
  Popup *topModalPopup() const;

  bool fillTerminal() const { return m_fillTerminal; }
  void setFillTerminal(bool fill);

  RendererMode rendererMode() const { return m_rendererMode; }
  void setRendererMode(RendererMode mode);
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

  void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
  void useAlternateScreenChanged();
  void fillTerminalChanged();
  void rendererModeChanged();
  void clipboardModeChanged();
  void mouseReportingChanged();
  void clipboardCopyRequested(const QString& text);
  void focusedItemChanged();
  void hoveredItemChanged();
  void mouseMotionRequestedChanged(bool requested);

private:
  TerminalEngine *m_engine = nullptr;
  QPointer<Item> m_focusedItem;
  QPointer<Item> m_hoveredItem;
  QHash<QObject*, QMetaObject::Connection> m_mouseMotionRequesters;
  QList<Popup*> m_popups;
  quint64 m_popupSerial = 0;
  bool m_useAlternateScreen = true;
  bool m_fillTerminal = true;
  bool m_fillTerminalExplicit = false;
  RendererMode m_rendererMode = RendererMode::Frame;
  ClipboardMode m_clipboardMode = ClipboardMode::Osc52;
  bool m_mouseReporting = true;
  bool m_mouseReportingExplicit = false;
};

} // namespace qtui
