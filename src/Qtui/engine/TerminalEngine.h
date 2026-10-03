#pragma once

#include <QObject>
#include <QPointer>
#include <QSocketNotifier>
#include <atomic>
#include "../primitives/TerminalDevice.h"
#include "../primitives/InputDecoder.h"
#include "Renderer.h"

class TestScrollView;

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
    void setMouseReporting(bool enable);

    int terminalWidth() const { return m_device.getWidth(); }
    int terminalHeight() const { return m_device.getHeight(); }

    void print(const QString& text);
    void append(qtui::Item* item);
    void copyToClipboard(const QString& text);
    static std::string osc52Sequence(const QString& text);

private slots:
    void readTerminalData();
    void readResizeSignal();
    void renderFrame();

private:
    friend class ::TestScrollView;

    void startRenderer();
    void setMouseMotionReportingEnabled(bool enabled);
    void handleResize();
    void handleKeyInput(int key, const QString& text, int modifiers, bool paste = false);
    void handleMouseInput(int button, int x, int y, bool release, bool motion = false,
                          int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);
    void cycleFocus(bool forward, Item* scope = nullptr);
    class Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;

    TerminalDevice m_device;
    InputDecoder m_inputDecoder;
    Renderer* m_renderer = nullptr;
    bool m_rendererStarted = false;
    bool m_mouseMotionReportingEnabled = false;
    
    QSocketNotifier* m_notifier = nullptr;
    QPointer<Terminal> m_rootWindow;
    
    std::atomic<bool> m_renderPending{false};
};

} // namespace qtui
