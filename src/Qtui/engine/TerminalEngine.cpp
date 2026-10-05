#include "TerminalEngine.h"
#include "InputDecoder.h"
#include "Renderer.h"
#include "FrameRenderer.h"
#include "../items/Item.h"
#include "ItemPaintTraversal_p.h"
#include "../items/Terminal.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include "InputRouter_p.h"
#include "TerminalMessageRouter_p.h"
#include "../primitives/TextGridBuffer.h"
#include "Keys.h"
#include "Shortcut.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QSet>
#include <QThread>
#include <QPointer>
#include <QtGlobal>
#include <algorithm>
#include <iostream>
#include <cstring>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#endif

namespace qtui {

// One line per committed frame. Off by default; QTUI_FRAME_STATS=1 enables it.
Q_LOGGING_CATEGORY(lcQtuiFrames, "qtui.frames", QtWarningMsg)

#ifdef Q_OS_UNIX
class TerminalResizeBridge : public QObject {
    Q_OBJECT
public:
    static TerminalResizeBridge* instance() {
        if (!qApp || QThread::currentThread() != qApp->thread())
            return nullptr;
        static QPointer<TerminalResizeBridge> bridge;
        if (!bridge)
            bridge = new TerminalResizeBridge(qApp);
        return bridge.data();
    }

    void registerEngine(TerminalEngine* engine) {
        // Ensure this is called on the application thread
        if (!qApp || QThread::currentThread() != qApp->thread()) {
            qWarning("TerminalEngine must be created on the QCoreApplication thread.");
            return;
        }

        if (!m_engines.contains(engine)) {
            m_engines.insert(engine);
            if (m_engines.size() == 1) {
                setup();
            }
        }
    }

    void unregisterEngine(TerminalEngine* engine) {
        if (!qApp || QThread::currentThread() != qApp->thread()) return;
        m_engines.remove(engine);
        if (m_engines.isEmpty())
            teardown();
    }

signals:
    void sigwinchReceived();

private slots:
    void handleSocketNotification() {
        char buf[128];
        ssize_t bytesRead;
        while ((bytesRead = ::read(sigwinchFd[0], buf, sizeof(buf))) > 0) {
            // Drain the pipe completely
        }
        emit sigwinchReceived();
    }

private:
    explicit TerminalResizeBridge(QObject* parent = nullptr) : QObject(parent) {}
    ~TerminalResizeBridge() override { teardown(); }
    
    void setup() {
        if (m_installed) return; // Retained for process lifetime

        if (::pipe(sigwinchFd) != 0) {
            sigwinchFd[0] = -1;
            sigwinchFd[1] = -1;
            return;
        }

        for (int i = 0; i < 2; ++i) {
            int flags = ::fcntl(sigwinchFd[i], F_GETFD);
            if (flags != -1) {
                ::fcntl(sigwinchFd[i], F_SETFD, flags | FD_CLOEXEC);
            }
            flags = ::fcntl(sigwinchFd[i], F_GETFL);
            if (flags != -1) {
                ::fcntl(sigwinchFd[i], F_SETFL, flags | O_NONBLOCK);
            }
        }

        struct sigaction sa;
        sa.sa_handler = TerminalResizeBridge::sigwinchHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        
        if (::sigaction(SIGWINCH, &sa, &m_oldAction) == 0) {
            m_installed = true;
            m_notifier = new QSocketNotifier(sigwinchFd[0], QSocketNotifier::Read, this);
            connect(m_notifier, &QSocketNotifier::activated, this, &TerminalResizeBridge::handleSocketNotification);
        } else {
            ::close(sigwinchFd[0]);
            ::close(sigwinchFd[1]);
            sigwinchFd[0] = -1;
            sigwinchFd[1] = -1;
        }
    }

    void teardown() {
        if (m_notifier) {
            m_notifier->setEnabled(false);
            delete m_notifier;
            m_notifier = nullptr;
        }
        if (m_installed) {
            ::sigaction(SIGWINCH, &m_oldAction, nullptr);
            m_installed = false;
        }
        for (int& fd : sigwinchFd) {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }
    }

    static void sigwinchHandler(int) {
        int savedErrno = errno;
        if (sigwinchFd[1] != -1) {
            char c = 1;
            ssize_t res = ::write(sigwinchFd[1], &c, 1);
            (void)res;
        }
        errno = savedErrno;
    }

    static int sigwinchFd[2];
    QSet<TerminalEngine*> m_engines;
    QSocketNotifier* m_notifier = nullptr;
    struct sigaction m_oldAction;
    bool m_installed = false;
};

int TerminalResizeBridge::sigwinchFd[2] = {-1, -1};
#endif


static bool environmentFlagEnabled(const char* name) {
  const QByteArray value = qgetenv(name).trimmed().toLower();
  return !value.isEmpty() && value != "0" && value != "false" && value != "off";
}

static bool environmentRequestsTestSession() {
  return !qgetenv("QTUI_SCRIPT").trimmed().isEmpty()
      || !qgetenv("QTUI_EVAL").trimmed().isEmpty()
      || environmentFlagEnabled("QTUI_INSPECT")
      || environmentFlagEnabled("QTUI_TEST_EMBEDDED_SCENARIO");
}

TerminalEngine::TerminalEngine(Terminal *rootWindow, QObject *parent)
    : QObject(parent), m_rootWindow(rootWindow), m_router(std::make_unique<InputRouter>(rootWindow)) {
  TerminalMessageRouter::registerEngine();
  m_terminalOutputEnabled = !environmentRequestsTestSession()
      || environmentFlagEnabled("QTUI_TEST_LIVE");
  m_frameStatsLogging = environmentFlagEnabled("QTUI_FRAME_STATS");
  if (m_frameStatsLogging)
      const_cast<QLoggingCategory&>(lcQtuiFrames()).setEnabled(QtDebugMsg, true);

  // Frame pacing: coalesce bursts of invalidations into at most maxFrameRate
  // commits per second. Test sessions stay uncapped so latency assertions
  // measure the framework, not the pacer.
  m_paceTimer.setSingleShot(true);
  m_paceTimer.setTimerType(Qt::PreciseTimer);
  connect(&m_paceTimer, &QTimer::timeout, this, &TerminalEngine::renderFrame);
  bool fpsValid = false;
  const int envFps = qgetenv("QTUI_MAX_FPS").trimmed().toInt(&fpsValid);
  setMaxFrameRate(fpsValid && envFps >= 0 ? envFps : (environmentRequestsTestSession() ? 0 : 60));
  bool limitValid = false;
  const qlonglong envLimit = qgetenv("QTUI_OUTPUT_QUEUE_LIMIT").trimmed().toLongLong(&limitValid);
  if (limitValid && envLimit >= 0)
      m_outputQueueLimit = static_cast<size_t>(envLimit);
  m_device.setDrainedCallback([this] {
      if (!m_renderDeferredForOutput)
          return;
      m_renderDeferredForOutput = false;
      QMetaObject::invokeMethod(this, &TerminalEngine::renderFrame, Qt::QueuedConnection);
  });

  // Setup InputDecoder callbacks
  m_inputDecoder.setKeyCallback([this](int key, const QString& text, int modifiers) {
      handleKeyInput(key, text, modifiers);
  });
  m_inputDecoder.setPasteCallback([this](const QString& text) {
      handleKeyInput(0, text, 0, true);
  });
  m_inputDecoder.setDetailedMouseCallback([this](const InputDecoder::MouseInput& input) {
      handleMouseInput(input.button, input.x, input.y, input.release, input.motion,
                       input.modifiers, input.wheelDeltaX, input.wheelDeltaY);
  });
  if (m_rootWindow) {
      connect(m_rootWindow, &Terminal::mouseMotionRequestedChanged,
              this, &TerminalEngine::setMouseMotionReportingEnabled);
  }

  if (m_terminalOutputEnabled) {
    m_device.setAsyncOutput(true);
    m_device.enableRawMode();
    m_device.hideCursor();
  }

#ifdef Q_OS_UNIX
  const bool stdoutIsTty = ::isatty(STDOUT_FILENO) == 1;
#else
  const bool stdoutIsTty = true;
#endif
  m_capabilities = TerminalCapabilities::detect(QProcessEnvironment::systemEnvironment(), stdoutIsTty);

  m_renderer = new FrameRenderer();
  m_renderer->setOutputEnabled(m_terminalOutputEnabled);
  m_renderer->setCapabilities(m_capabilities);

  // Enable bracketed paste. Enable mouse reporting only if enabled on root window.
  if (m_terminalOutputEnabled) {
    m_device.write("\033[?2004h");
    if (!m_rootWindow || m_rootWindow->mouseReporting()) {
      m_device.write("\033[?1003l\033[?1000h\033[?1002h\033[?1006h");
    } else {
      m_device.write("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
    }
  }
  setMouseMotionReportingEnabled(m_rootWindow && m_rootWindow->hasMouseMotionRequests());
  if (m_rootWindow && !m_rootWindow->title().isEmpty()) {
    setTitle(m_rootWindow->title());
  }

  handleResize();

  m_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
  connect(m_notifier, &QSocketNotifier::activated, this,
          &TerminalEngine::readTerminalData);

#ifdef Q_OS_UNIX
  if (TerminalResizeBridge* bridge = TerminalResizeBridge::instance()) {
      connect(bridge, &TerminalResizeBridge::sigwinchReceived,
              this, &TerminalEngine::readResizeSignal);
      bridge->registerEngine(this);
  } else {
      qWarning("TerminalEngine must be created on the QCoreApplication thread.");
  }
#endif

  requestRender();
}

TerminalEngine::~TerminalEngine() {
#ifdef Q_OS_UNIX
  if (TerminalResizeBridge* bridge = TerminalResizeBridge::instance())
      bridge->unregisterEngine(this);
#endif
  stop();
  delete m_renderer;
  m_renderer = nullptr;
  TerminalMessageRouter::unregisterEngine();
}

void TerminalEngine::stop() {
  m_paceTimer.stop();
  if (m_terminalOutputEnabled)
    m_device.write("\033[?1000l\033[?1002l\033[?1003l\033[?1006l\033[?2004l");
  if (m_renderer && m_rendererStarted) {
      m_renderer->stop(m_rootWindow, m_device);
      m_rendererStarted = false;
  }
  if (m_terminalOutputEnabled) {
    m_device.showCursor();
    const bool flushed = m_device.finishOutput();
    m_device.disableRawMode();
    if (!flushed)
        emit renderFailed(QStringLiteral("Terminal output could not be flushed during shutdown; undelivered bytes were discarded"));
  }
  if (m_alternateScreenMessageSession) {
      m_alternateScreenMessageSession = false;
      TerminalMessageRouter::endAlternateScreenSession();
  }
}

void TerminalEngine::requestRender() {
  if (m_renderPending.exchange(true))
    return;
  if (QThread::currentThread() == thread() && m_frameIntervalMs > 0 && m_lastCommit.isValid()) {
    const qint64 sinceCommit = m_lastCommit.elapsed();
    if (sinceCommit < m_frameIntervalMs) {
      m_paceTimer.start(static_cast<int>(m_frameIntervalMs - sinceCommit));
      return;
    }
  }
  QMetaObject::invokeMethod(this, &TerminalEngine::renderFrame,
                            Qt::QueuedConnection);
}

void TerminalEngine::setMaxFrameRate(int framesPerSecond) {
  m_maxFrameRate = std::max(0, framesPerSecond);
  m_frameIntervalMs = m_maxFrameRate > 0 ? std::max(1, 1000 / m_maxFrameRate) : 0;
  if (m_frameIntervalMs == 0 && m_paceTimer.isActive()) {
    m_paceTimer.stop();
    QMetaObject::invokeMethod(this, &TerminalEngine::renderFrame, Qt::QueuedConnection);
  }
}

int TerminalEngine::terminalWidth() const {
  return m_testViewportSize.isValid() ? m_testViewportSize.width() : m_device.getWidth();
}

int TerminalEngine::terminalHeight() const {
  return m_testViewportSize.isValid() ? m_testViewportSize.height() : m_device.getHeight();
}

std::optional<TextGridBuffer> TerminalEngine::currentFrame() const {
  if (!m_renderer || !m_renderer->currentBuffer())
    return std::nullopt;
  return *m_renderer->currentBuffer();
}

void TerminalEngine::setTestViewportSize(const QSize& size) {
  if (!size.isValid() || size.isEmpty() || m_testViewportSize == size)
    return;
  m_testViewportSize = size;
  handleResize();
  requestRender();
}

void TerminalEngine::setMouseMotionReportingEnabled(bool enabled) {
  if (m_mouseMotionReportingEnabled == enabled)
    return;
  m_mouseMotionReportingEnabled = enabled;
  if (!m_terminalOutputEnabled)
    return;
  if (enabled) {
    m_device.write("\033[?1003h");
  } else {
    // Some terminals treat mouse tracking modes as mutually exclusive, so
    // explicitly restore button-held motion after disabling all-motion mode.
    m_device.write("\033[?1003l\033[?1002h");
  }
}

void TerminalEngine::setMouseReporting(bool enable) {
  if (!m_terminalOutputEnabled)
    return;
  if (enable) {
    m_device.write("\033[?1000h\033[?1002h\033[?1006h");
    setMouseMotionReportingEnabled(m_rootWindow && m_rootWindow->hasMouseMotionRequests());
  } else {
    m_device.write("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
  }
}

void TerminalEngine::copyToClipboard(const QString& text) {
  if (m_terminalOutputEnabled)
    m_device.write(osc52Sequence(text));
}

std::string TerminalEngine::osc52Sequence(const QString& text) {
  const QByteArray encoded = text.toUtf8().toBase64();
  return "\033]52;c;" + encoded.toStdString() + "\a";
}

void TerminalEngine::setTitle(const QString& title) {
  if (m_title == title)
    return;
  m_title = title;
  if (m_terminalOutputEnabled && !m_title.isEmpty()) {
    m_device.write(titleSequence(m_title));
  }
}

std::string TerminalEngine::titleSequence(const QString& text) {
  return "\033]0;" + text.toUtf8().toStdString() + "\a";
}

void TerminalEngine::handleResize() {
  int w = terminalWidth();
  int h = terminalHeight();
  if (m_renderer) {
      m_renderer->resize(w, h);
  }
  if (m_rootWindow) {
    emit m_rootWindow->terminalSizeChanged();
    if (m_rootWindow->fillTerminal()) {
      emit m_rootWindow->widthChanged();
      emit m_rootWindow->heightChanged();
      m_rootWindow->markDirty();
    }
  }
}

void TerminalEngine::readResizeSignal() {
  handleResize();
  requestRender();
}

void TerminalEngine::readTerminalData() {
  char buffer[1024];
  const ssize_t n = read(STDIN_FILENO, buffer, sizeof(buffer));
  if (n == 0) {
      if (m_notifier) m_notifier->setEnabled(false);
      return;
  }
  if (n < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
          return;
      if (m_notifier) m_notifier->setEnabled(false);
      qWarning("Terminal input failed: %s", std::strerror(errno));
      return;
  }

  m_inputDecoder.decode(buffer, n);
}

void TerminalEngine::handleKeyInput(int key, const QString& text, int modifiers, bool paste) {
  if (m_router) m_router->routeKey(key, text, modifiers, paste);
}

void TerminalEngine::handleMouseInput(int button, int x, int y, bool release, bool motion,
                                      int modifiers, int wheelDeltaX, int wheelDeltaY) {
  if (m_router) m_router->routeMouse(button, x, y, release, motion, modifiers, wheelDeltaX, wheelDeltaY);
}

void TerminalEngine::cycleFocus(bool forward, Item* scope) {
  if (m_router) m_router->cycleFocus(forward, scope);
}

Item* TerminalEngine::hitTestItem(Item* item, int absX, int absY, int parentAbsX, int parentAbsY) const {
  return m_router ? m_router->hitTestItem(item, absX, absY, parentAbsX, parentAbsY) : nullptr;
}

void TerminalEngine::renderFrame() {
  m_paceTimer.stop();
  if (m_device.pendingBytes() > m_outputQueueLimit) {
    // The terminal has not consumed earlier frames yet. Keep the request
    // latched; the device calls back once the queue has drained, and the next
    // frame is diffed against the last committed buffer so nothing is lost.
    m_renderPending = true;
    m_renderDeferredForOutput = true;
    ++m_frameStatsTotals.skippedForBackpressure;
    return;
  }
  m_renderPending = false;
  if (!m_rootWindow) {
    emit renderFailed(QStringLiteral("Terminal root was destroyed before rendering"));
    return;
  }

  if (!m_renderer) {
      emit renderFailed(QStringLiteral("No renderer is attached to TerminalEngine"));
      return;
  }

  startRenderer();
  if (!m_renderer->renderFrame(m_rootWindow, m_device)) {
      emit renderFailed(QStringLiteral("Renderer failed to commit the frame"));
      return;
  }

  m_lastCommit.restart();
  FrameStats stats = m_renderer->lastFrameStats();
  stats.sequence = ++m_frameSequence;
  m_lastFrameStats = stats;
  m_frameStatsTotals.frames += 1;
  m_frameStatsTotals.paintMicrosTotal += stats.paintMicros;
  m_frameStatsTotals.paintMicrosMax = std::max(m_frameStatsTotals.paintMicrosMax, stats.paintMicros);
  m_frameStatsTotals.diffMicrosTotal += stats.diffMicros;
  m_frameStatsTotals.writeMicrosTotal += stats.writeMicros;
  m_frameStatsTotals.bytesTotal += stats.diffBytes;
  m_frameStatsTotals.dirtyDuringPaintTotal += stats.dirtyDuringPaint;
  if (m_frameStatsLogging) {
      qCDebug(lcQtuiFrames, "frame %llu paint=%lldus diff=%lldus write=%lldus bytes=%lld dirtyDuringPaint=%d",
              static_cast<unsigned long long>(stats.sequence),
              static_cast<long long>(stats.paintMicros),
              static_cast<long long>(stats.diffMicros),
              static_cast<long long>(stats.writeMicros),
              static_cast<long long>(stats.diffBytes),
              stats.dirtyDuringPaint);
  }

  emit frameRendered(stats.sequence);
}

void TerminalEngine::setRenderer(Renderer* renderer) {
    if (m_renderer == renderer) return;
    if (m_renderer) {
        if (m_rendererStarted) {
            m_renderer->stop(m_rootWindow, m_device);
            m_rendererStarted = false;
            if (m_alternateScreenMessageSession) {
                // Restore normal terminal processing before replaying messages
                // accumulated by the renderer being replaced.
                if (m_terminalOutputEnabled) {
                    m_device.showCursor();
                    m_device.disableRawMode();
                }
                m_alternateScreenMessageSession = false;
                TerminalMessageRouter::endAlternateScreenSession();
                if (m_terminalOutputEnabled) {
                    m_device.enableRawMode();
                    m_device.hideCursor();
                }
            }
        }
        delete m_renderer;
    }
    m_renderer = renderer;
    m_rendererStarted = false;
    if (m_renderer) {
        m_renderer->setOutputEnabled(m_terminalOutputEnabled);
        m_renderer->setCapabilities(m_capabilities);
        m_renderer->resize(terminalWidth(), terminalHeight());
        requestRender();
    }
}

void TerminalEngine::setCapabilities(const TerminalCapabilities& capabilities) {
    m_capabilities = capabilities;
    if (m_renderer)
        m_renderer->setCapabilities(m_capabilities);
    requestRender();
}

void TerminalEngine::setTerminalOutputEnabled(bool enabled) {
  if (m_terminalOutputEnabled == enabled)
    return;

  if (m_renderer && m_rendererStarted) {
    m_renderer->stop(m_rootWindow, m_device);
    m_rendererStarted = false;
  }
  if (m_alternateScreenMessageSession) {
    m_alternateScreenMessageSession = false;
    TerminalMessageRouter::endAlternateScreenSession();
  }

  if (!enabled) {
    m_device.write("\033[?1000l\033[?1002l\033[?1003l\033[?1006l\033[?2004l");
    m_device.showCursor();
    const bool flushed = m_device.finishOutput();
    m_device.disableRawMode();
    if (!flushed)
        emit renderFailed(QStringLiteral("Terminal output could not be flushed when disabling output; undelivered bytes were discarded"));
  } else {
    m_device.setAsyncOutput(true);
    m_device.enableRawMode();
    m_device.hideCursor();
    m_device.write("\033[?2004h");
    if (!m_rootWindow || m_rootWindow->mouseReporting())
      m_device.write("\033[?1003l\033[?1000h\033[?1002h\033[?1006h");
  }

  m_terminalOutputEnabled = enabled;
  if (m_renderer)
    m_renderer->setOutputEnabled(enabled);
  requestRender();
}

void TerminalEngine::print(const QString& text) {
    if (m_renderer) {
        startRenderer();
        m_renderer->print(m_device, text.toStdString());
    }
}

void TerminalEngine::startRenderer() {
    if (!m_renderer || m_rendererStarted) return;
    if (m_renderer->usesAlternateScreen(m_rootWindow)) {
        TerminalMessageRouter::beginAlternateScreenSession();
        m_alternateScreenMessageSession = true;
    }
    m_renderer->start(m_rootWindow, m_device);
    m_rendererStarted = true;
}

void TerminalEngine::append(qtui::Item* item) {
    if (!item) return;
    int w = item->width() > 0 ? item->width() : m_device.getWidth();
    int h = item->height() > 0 ? item->height() : item->implicitHeight();
    if (h <= 0) h = 1;
    
    TextGridBuffer buffer;
    buffer.resize(w, h);
    ItemPaintTraversal::paintSubtree(item, buffer, 0, 0);
    
    print(QString::fromStdString(buffer.toAnsiString()));
}

} // namespace qtui

#include "TerminalEngine.moc"
