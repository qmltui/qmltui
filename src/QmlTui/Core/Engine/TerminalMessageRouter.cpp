#include <QmlTui/Core/Engine/TerminalMessageRouter_p.h>

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QThread>
#include <QVarLengthArray>
#include <QWaitCondition>
#include <QtEnvironmentVariables>
#include <QtLogging>

#include <atomic>
#include <cstdio>
#include <exception>
#include <utility>

namespace qmltui::TerminalMessageRouter {
namespace {

// Tracks threads currently executing inside a log sink or log-file write without
// C++ thread_local storage (which interacts poorly with WINDOWS_EXPORT_ALL_SYMBOLS
// DLLs on MSVC) and without allocating during early startup or shutdown.
class ActiveCallbackTracker {
public:
    bool containsCurrentThread() {
        const Qt::HANDLE self = QThread::currentThreadId();
        for (const auto& slot : m_fastSlots) {
            if (slot.load(std::memory_order_acquire) == self) return true;
        }
        if (m_overflowCount.load(std::memory_order_acquire) == 0) return false;
        QMutexLocker lock(&m_overflowMutex);
        return m_overflowThreads.contains(self);
    }

    void enterCurrentThread() {
        const Qt::HANDLE self = QThread::currentThreadId();
        for (auto& slot : m_fastSlots) {
            Qt::HANDLE expected = nullptr;
            if (slot.compare_exchange_strong(expected, self, std::memory_order_acq_rel)) {
                return;
            }
        }
        QMutexLocker lock(&m_overflowMutex);
        m_overflowThreads.push_back(self);
        m_overflowCount.fetch_add(1, std::memory_order_release);
    }

    void leaveCurrentThread() {
        const Qt::HANDLE self = QThread::currentThreadId();
        for (auto& slot : m_fastSlots) {
            Qt::HANDLE expected = self;
            if (slot.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel)) {
                return;
            }
        }
        QMutexLocker lock(&m_overflowMutex);
        for (qsizetype i = m_overflowThreads.size() - 1; i >= 0; --i) {
            if (m_overflowThreads[i] == self) {
                m_overflowThreads.remove(i);
                m_overflowCount.fetch_sub(1, std::memory_order_release);
                break;
            }
        }
    }

private:
    std::atomic<Qt::HANDLE> m_fastSlots[64]{};
    std::atomic<int> m_overflowCount{0};
    QMutex m_overflowMutex;
    QVarLengthArray<Qt::HANDLE, 8> m_overflowThreads;
};

ActiveCallbackTracker& callbackTracker() {
    static ActiveCallbackTracker* tracker = new ActiveCallbackTracker;
    return *tracker;
}

class ScopedCallbackGuard {
public:
    ScopedCallbackGuard() { callbackTracker().enterCurrentThread(); }
    ~ScopedCallbackGuard() { callbackTracker().leaveCurrentThread(); }
    ScopedCallbackGuard(const ScopedCallbackGuard&) = delete;
    ScopedCallbackGuard& operator=(const ScopedCallbackGuard&) = delete;
};

struct PresentationRecord {
    uint64_t seq = 0;
    QtMessage message;
    bool eligible = false;
    uint64_t registrationId = 0;
};

enum class RegistrationState { None, Active, Retiring };

struct RouterState {
    QMutex mutex;
    QWaitCondition retirementWaitCondition;
    int engineCount = 0;
    int alternateScreenSessionCount = 0;
    int deferredDefaultStderrSessionCount = 0;
    bool handlerInstalled = false;
    QtMessageHandler previousHandler = nullptr;

    RegistrationState regState = RegistrationState::None;
    uint64_t activeRegistrationId = 0;
    QtMessageSink diagnosticSink;
    QtMessageSink presentationSink;
    bool hasExplicitPresentationSink = false;

    int activeDiagnosticDeliveries = 0;
    int activePresentationDeliveries = 0;
    QVarLengthArray<Qt::HANDLE, 4> activeDeliveryThreads;

    uint64_t nextSequenceNumber = 0;
    QList<PresentationRecord> presentationQueue;
    bool isDraining = false;

    void beginDelivery(bool diagnostic) {
        if (diagnostic)
            ++activeDiagnosticDeliveries;
        else
            ++activePresentationDeliveries;
        activeDeliveryThreads.push_back(QThread::currentThreadId());
    }

    void endDelivery(bool diagnostic) {
        if (diagnostic)
            --activeDiagnosticDeliveries;
        else
            --activePresentationDeliveries;
        const Qt::HANDLE self = QThread::currentThreadId();
        for (qsizetype i = activeDeliveryThreads.size() - 1; i >= 0; --i) {
            if (activeDeliveryThreads[i] == self) {
                activeDeliveryThreads.remove(i);
                break;
            }
        }
        if (regState == RegistrationState::Retiring) {
            retirementWaitCondition.wakeAll();
        }
    }

    int activeDeliveryCountExcludingSelf() const {
        const Qt::HANDLE self = QThread::currentThreadId();
        int otherCount = 0;
        for (Qt::HANDLE tid : activeDeliveryThreads) {
            if (tid != self) ++otherCount;
        }
        return otherCount;
    }
};

RouterState& state() {
    static RouterState* routerState = new RouterState;
    return *routerState;
}

void appendToConfiguredLogFile(const QtMessage& msg) {
    const QString path = qEnvironmentVariable("QMLTUI_LOG_FILE").trimmed();
    if (path.isEmpty()) return;

    struct LogFileState {
        QMutex mutex;
        QString openPath;
        QFile file;
    };
    static LogFileState* logState = new LogFileState;

    QMutexLocker lock(&logState->mutex);
    const ScopedCallbackGuard callbackGuard;
    if (logState->openPath != path || !logState->file.isOpen()) {
        if (logState->file.isOpen()) logState->file.close();
        logState->file.setFileName(path);
        logState->openPath = path;
        (void)logState->file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    if (logState->file.isOpen()) {
        QByteArray line = msg.formattedMessage().toUtf8();
        line.append('\n');
        logState->file.write(line);
        logState->file.flush();
    }
}

bool isPresentationDeferred(const RouterState& router) {
    if (router.regState == RegistrationState::None) {
        return router.deferredDefaultStderrSessionCount > 0;
    }
    return router.alternateScreenSessionCount > 0;
}

void drainPresentationQueue(RouterState& router, QMutexLocker<QMutex>& lock) {
    if (router.isDraining) return;

    router.isDraining = true;
    while (!isPresentationDeferred(router) && !router.presentationQueue.isEmpty()) {
        if (!router.presentationQueue.first().eligible) break;

        PresentationRecord rec = router.presentationQueue.takeFirst();
        QtMessageSink sink = nullptr;
        if (router.regState == RegistrationState::Active) {
            if (rec.registrationId == router.activeRegistrationId &&
                router.hasExplicitPresentationSink)
                sink = router.presentationSink;
        } else if (router.regState == RegistrationState::None) {
            if (rec.registrationId == 0) sink = stderrPresentationSink();
        }

        if (sink) {
            router.beginDelivery(false);
        }

        lock.unlock();

        if (sink) {
            const ScopedCallbackGuard callbackGuard;
            try {
                sink(rec.message);
            } catch (const std::exception& e) {
                std::fprintf(stderr, "qmltui: presentation sink exception: %s\n", e.what());
                std::fflush(stderr);
            } catch (...) {
                std::fprintf(stderr, "qmltui: presentation sink unknown exception\n");
                std::fflush(stderr);
            }
        }

        lock.relock();

        if (sink) {
            router.endDelivery(false);
        }
    }
    router.isDraining = false;
}

void routeMessage(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (callbackTracker().containsCurrentThread()) {
        const QString formatted = qFormatLogMessage(type, context, message);
        std::fprintf(stderr, "%s\n", qPrintable(formatted));
        std::fflush(stderr);
        return;
    }

    RouterState& router = state();

    if (type == QtFatalMsg) {
        QtMessage msg;
        msg.type = type;
        msg.message = message;
        msg.timestampUtc = QDateTime::currentDateTimeUtc();
        if (context.category) msg.category = QByteArray(context.category);
        if (context.file) msg.file = QByteArray(context.file);
        if (context.function) msg.function = QByteArray(context.function);
        msg.line = context.line;
        msg.formattedText = qFormatLogMessage(type, context, message);

        QtMessageSink diagSink = nullptr;
        QtMessageSink presSink = nullptr;
        {
            QMutexLocker lock(&router.mutex);
            if (router.regState == RegistrationState::Active) {
                diagSink = router.diagnosticSink;
                if (router.hasExplicitPresentationSink) presSink = router.presentationSink;
            } else {
                presSink = stderrPresentationSink();
            }
            if (diagSink) router.beginDelivery(true);
            if (presSink) router.beginDelivery(false);
        }

        {
            const ScopedCallbackGuard callbackGuard;
            if (diagSink) {
                try {
                    diagSink(msg);
                } catch (...) {}
            }
            if (presSink) {
                try {
                    presSink(msg);
                } catch (...) {}
            }
            std::fflush(stderr);
        }
        appendToConfiguredLogFile(msg);
        {
            QMutexLocker lock(&router.mutex);
            if (presSink) router.endDelivery(false);
            if (diagSink) router.endDelivery(true);
        }
        return;
    }

    uint64_t seq = 0;
    QtMessage msg;
    QtMessageSink diagSink = nullptr;
    bool hasPresentation = false;
    uint64_t regId = 0;

    {
        QMutexLocker lock(&router.mutex);
        seq = ++router.nextSequenceNumber;
        msg.type = type;
        msg.message = message;
        msg.timestampUtc = QDateTime::currentDateTimeUtc();
        if (context.category) msg.category = QByteArray(context.category);
        if (context.file) msg.file = QByteArray(context.file);
        if (context.function) msg.function = QByteArray(context.function);
        msg.line = context.line;
        msg.formattedText = qFormatLogMessage(type, context, message);

        regId = router.activeRegistrationId;
        if (router.regState == RegistrationState::Active) {
            diagSink = router.diagnosticSink;
            hasPresentation = router.hasExplicitPresentationSink;
        } else if (router.regState == RegistrationState::None) {
            hasPresentation = true;
            regId = 0;
        }

        if (hasPresentation) {
            constexpr qsizetype kMaxPresentationMessages = 500;
            if (router.presentationQueue.size() >= kMaxPresentationMessages) {
                router.presentationQueue.removeFirst();
            }
            router.presentationQueue.append(PresentationRecord{seq, msg, false, regId});
        }

        if (diagSink) {
            router.beginDelivery(true);
        }
    }

    appendToConfiguredLogFile(msg);

    if (diagSink) {
        const ScopedCallbackGuard callbackGuard;
        try {
            diagSink(msg);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "qmltui: diagnostic sink exception: %s\n", e.what());
            std::fflush(stderr);
        } catch (...) {
            std::fprintf(stderr, "qmltui: diagnostic sink unknown exception\n");
            std::fflush(stderr);
        }
    }

    {
        QMutexLocker lock(&router.mutex);
        if (diagSink) {
            router.endDelivery(true);
        }

        if (hasPresentation) {
            for (auto& rec : router.presentationQueue) {
                if (rec.seq == seq) {
                    rec.eligible = true;
                    break;
                }
            }
        }

        if (!isPresentationDeferred(router)) {
            drainPresentationQueue(router, lock);
        }
    }
}

} // namespace

void registerEngine() {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    ++router.engineCount;
    if (!router.handlerInstalled) {
        router.previousHandler = qInstallMessageHandler(routeMessage);
        router.handlerInstalled = true;
    }
}

void unregisterEngine() {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.engineCount <= 0) return;

    --router.engineCount;
    if (router.engineCount == 0) {
        router.alternateScreenSessionCount = 0;
        router.deferredDefaultStderrSessionCount = 0;
        drainPresentationQueue(router, lock);
        if (router.regState == RegistrationState::None && router.handlerInstalled) {
            qInstallMessageHandler(router.previousHandler);
            router.previousHandler = nullptr;
            router.handlerInstalled = false;
        }
    }
}

void beginAlternateScreenSession(bool deferDefaultStderr) {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    ++router.alternateScreenSessionCount;
    if (deferDefaultStderr) {
        ++router.deferredDefaultStderrSessionCount;
    }
}

void endAlternateScreenSession(bool deferDefaultStderr) {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.alternateScreenSessionCount <= 0) return;

    --router.alternateScreenSessionCount;
    if (deferDefaultStderr && router.deferredDefaultStderrSessionCount > 0) {
        --router.deferredDefaultStderrSessionCount;
    }
    if (!isPresentationDeferred(router)) {
        drainPresentationQueue(router, lock);
    }
}

bool installRegistration(uint64_t registrationId, QtMessageSink diagnosticSink,
                         QtMessageSink presentationSink, bool hasPresentationSink) {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.regState != RegistrationState::None) return false;

    router.presentationQueue.clear();
    router.regState = RegistrationState::Active;
    router.activeRegistrationId = registrationId;
    router.diagnosticSink = std::move(diagnosticSink);
    router.presentationSink = std::move(presentationSink);
    router.hasExplicitPresentationSink = hasPresentationSink;

    if (!router.handlerInstalled) {
        router.previousHandler = qInstallMessageHandler(routeMessage);
        router.handlerInstalled = true;
    }

    return true;
}

void resetRegistration(uint64_t registrationId) {
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.activeRegistrationId != registrationId ||
        router.regState != RegistrationState::Active) {
        return;
    }

    router.regState = RegistrationState::Retiring;
    router.presentationQueue.clear();

    while (router.activeDeliveryCountExcludingSelf() > 0) {
        router.retirementWaitCondition.wait(&router.mutex);
    }

    router.diagnosticSink = nullptr;
    router.presentationSink = nullptr;
    router.hasExplicitPresentationSink = false;
    router.activeRegistrationId = 0;
    router.regState = RegistrationState::None;

    if (router.engineCount == 0 && router.handlerInstalled) {
        qInstallMessageHandler(router.previousHandler);
        router.previousHandler = nullptr;
        router.handlerInstalled = false;
    }
}

QtMessageSink stderrPresentationSink() {
    return [](const QtMessage& msg) {
        RouterState& router = state();
        QtMessageHandler prev = nullptr;
        {
            QMutexLocker lock(&router.mutex);
            prev = router.previousHandler;
        }
        if (prev) {
            const QMessageLogContext context(
                msg.file.isNull() ? nullptr : msg.file.constData(), msg.line,
                msg.function.isNull() ? nullptr : msg.function.constData(),
                msg.category.isNull() ? nullptr : msg.category.constData());
            prev(msg.type, context, msg.message);
        } else {
            std::fprintf(stderr, "%s\n", qPrintable(msg.formattedMessage()));
            std::fflush(stderr);
        }
    };
}

} // namespace qmltui::TerminalMessageRouter
