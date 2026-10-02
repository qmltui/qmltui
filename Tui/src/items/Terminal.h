#pragma once

#include "Item.h"

namespace qtui {

class TerminalEngine;

class Terminal : public Item {
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

  bool fillTerminal() const { return m_fillTerminal; }
  void setFillTerminal(bool fill);

  RendererMode rendererMode() const { return m_rendererMode; }
  void setRendererMode(RendererMode mode);

  Item* focusedItem() const { return m_focusedItem; }
  void setFocusedItem(Item* item);

  void findFocusableItems(Item* root, QList<Item*>& list);

  int width() const override;
  int height() const override;

  void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
  void useAlternateScreenChanged();
  void fillTerminalChanged();
  void rendererModeChanged();

private:
  TerminalEngine *m_engine = nullptr;
  Item* m_focusedItem = nullptr;
  bool m_useAlternateScreen = true;
  bool m_fillTerminal = true;
  RendererMode m_rendererMode = RendererMode::Frame;
};

} // namespace qtui
