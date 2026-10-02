#pragma once

#include "Rectangle.h"

namespace qtui {

class TerminalEngine;
class Popup;

class Terminal : public Rectangle {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(bool useAlternateScreen READ useAlternateScreen WRITE setUseAlternateScreen NOTIFY useAlternateScreenChanged)
  Q_PROPERTY(bool fillTerminal READ fillTerminal WRITE setFillTerminal NOTIFY fillTerminalChanged)
  Q_PROPERTY(RendererMode rendererMode READ rendererMode WRITE setRendererMode NOTIFY rendererModeChanged)

public:
  enum class RendererMode {
      Frame,
      Stream
  };
  Q_ENUM(RendererMode)

  explicit Terminal(QObject *parent = nullptr);
  ~Terminal() override;

  Q_INVOKABLE void print(const QString& text);
  Q_INVOKABLE void append(qtui::Item* item);

  bool useAlternateScreen() const { return m_useAlternateScreen; }
  void setUseAlternateScreen(bool use);

  void registerPopup(Popup* popup);
  void unregisterPopup(Popup* popup);
  const QList<Popup*>& activePopups() const { return m_popups; }

  bool fillTerminal() const { return m_fillTerminal; }
  void setFillTerminal(bool fill);

  RendererMode rendererMode() const { return m_rendererMode; }
  void setRendererMode(RendererMode mode);

  Item* focusedItem() const { return m_focusedItem; }
  void setFocusedItem(Item* item);

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

private:
  TerminalEngine *m_engine = nullptr;
  Item* m_focusedItem = nullptr;
  QList<Popup*> m_popups;
  bool m_useAlternateScreen = true;
  bool m_fillTerminal = true;
  bool m_fillTerminalExplicit = false;
  RendererMode m_rendererMode = RendererMode::Frame;
};

} // namespace qtui
