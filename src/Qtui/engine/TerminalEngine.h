#pragma once

#include <QObject>
#include <QPointer>
#include <QSize>
#include <QSocketNotifier>
#include <atomic>
#include <optional>
#include <memory>
#include "../primitives/TerminalDevice.h"
#include "../primitives/InputDecoder.h"
#include "../primitives/TextGridBuffer.h"
#include "Renderer.h"

class TestScrollView;

namespace qtui {

class Terminal;
class Item;
class InputRouter;
class TerminalEngine : public QObject {
    Q_OBJECT

public:
    explicit TerminalEngine(Terminal* rootWindow, QObject* parent = nullptr);
    ~TerminalEngine() override;

    Q_INVOKABLE void requestRender();

    quint64 frameSequence() const { return m_frameSequence; }
    std::optional<TextGridBuffer> currentFrame() const;
    void setTestViewportSize(const QSize& size);

    void setRenderer(Renderer* renderer);
    Renderer* renderer() const { return m_renderer; }
    void setTerminalOutputEnabled(bool enabled);
    bool terminalOutputEnabled() const { return m_terminalOutputEnabled; }
    void stop();
    void setMouseReporting(bool enable);

    int terminalWidth() const;
    int terminalHeight() const;

    void print(const QString& text);
    void append(qtui::Item* item);
    void copyToClipboard(const QString& text);
    static std::string osc52Sequence(const QString& text);
    void setTitle(const QString& title);
    QString title() const { return m_title; }
    static std::string titleSequence(const QString& text);

    void handleKeyInput(int key, const QString& text, int modifiers, bool paste = false);
    void handleMouseInput(int button, int x, int y, bool release, bool motion = false,
                          int modifiers = 0, int wheelDeltaX = 0, int wheelDeltaY = 0);

signals:
    void frameRendered(quint64 sequence);
    void renderFailed(const QString& message);

private slots:
    void readTerminalData();
    void readResizeSignal();
    void renderFrame();

private:
    friend class ::TestScrollView;

    void startRenderer();
    void setMouseMotionReportingEnabled(bool enabled);
    void handleResize();
    void cycleFocus(bool forward, Item* scope = nullptr);
    class Item* hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const;

    TerminalDevice m_device;
    InputDecoder m_inputDecoder;
    Renderer* m_renderer = nullptr;
    bool m_rendererStarted = false;
    bool m_alternateScreenMessageSession = false;
    bool m_terminalOutputEnabled = true;
    bool m_mouseMotionReportingEnabled = false;
    
    QSocketNotifier* m_notifier = nullptr;
    QPointer<Terminal> m_rootWindow;
    
    std::atomic<bool> m_renderPending{false};
    QSize m_testViewportSize;
    quint64 m_frameSequence = 0;
    QString m_title;
    std::unique_ptr<InputRouter> m_router;
};

} // namespace qtui
