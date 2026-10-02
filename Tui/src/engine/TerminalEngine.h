#pragma once

#include <QObject>
#include <QSocketNotifier>
#include "../core/TerminalDevice.h"
#include "../core/InputDecoder.h"
#include "Renderer.h"

namespace qtui {

class Terminal;
class Item;

class TerminalEngine : public QObject {
    Q_OBJECT

public:
    explicit TerminalEngine(Terminal* rootWindow, QObject* parent = nullptr);
    ~TerminalEngine() override;

    Q_INVOKABLE void requestRender();

    void setRenderer(Renderer* renderer);

    int terminalWidth() const { return m_device.getWidth(); }
    int terminalHeight() const { return m_device.getHeight(); }

    void print(const QString& text);
    void append(qtui::Item* item);

private slots:
    void readTerminalData();
    void readResizeSignal();
    void renderFrame();

private:
    void startRenderer();
    void handleResize();
    void handleKeyInput(int key, const QString& text, int modifiers);
    void handleMouseInput(int button, int x, int y, bool release);
    void cycleFocus(bool forward);
    class MouseArea* hitTestMouseArea(Item* item, int absX, int absY, int parentAbsX, int parentAbsY);

    TerminalDevice m_device;
    InputDecoder m_inputDecoder;
    Renderer* m_renderer = nullptr;
    bool m_rendererStarted = false;
    
    QSocketNotifier* m_notifier = nullptr;
    QSocketNotifier* m_resizeNotifier = nullptr;
    Terminal* m_rootWindow = nullptr;
    
    bool m_renderPending = false;
};

} // namespace qtui
