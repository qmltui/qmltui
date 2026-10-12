#include <Qamui/Core/Engine/TerminalMessageRouter_p.h>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QWaitCondition>
#include <QtLogging>

#include <cstdio>
#include <exception>
#include <utility>

namespace qamui::TerminalMessageRouter {
namespace {

thread_local bool s_inCallback = false;

struct PresentationRecord
{
    uint64_t seq = 0;
    QtMessage message;
    bool eligible = false;
    uint64_t registrationId = 0;
};

enum class RegistrationState
{
    None,
    Active,
    Retiring
};

struct RouterState
{
    QMutex mutex;
    QWaitCondition retirementWaitCondition;
    int engineCount = 0;
    int alternateScreenSessionCount = 0;
    bool handlerInstalled = false;
    QtMessageHandler previousHandler = nullptr;

    RegistrationState regState = RegistrationState::None;
    uint64_t activeRegistrationId = 0;
    QtMessageSink diagnosticSink;
    QtMessageSink presentationSink;
    bool hasExplicitPresentationSink = false;

    int activeDiagnosticDeliveries = 0;
    int activePresentationDeliveries = 0;

    uint64_t nextSequenceNumber = 0;
    QList<PresentationRecord> presentationQueue;
    bool isDraining = false;
};

RouterState& state()
{
    static RouterState* routerState = new RouterState;
    return *routerState;
}

void drainPresentationQueue(RouterState& router, QMutexLocker<QMutex>& lock)
{
    if (router.isDraining)
        return;

    router.isDraining = true;
    while (router.alternateScreenSessionCount == 0 && !router.presentationQueue.isEmpty()) {
        if (!router.presentationQueue.first().eligible)
            break;

        PresentationRecord rec = router.presentationQueue.takeFirst();
        QtMessageSink sink = nullptr;
        if (router.regState == RegistrationState::Active) {
            if (rec.registrationId == router.activeRegistrationId && router.hasExplicitPresentationSink)
                sink = router.presentationSink;
        } else if (router.regState == RegistrationState::None) {
            if (rec.registrationId == 0)
                sink = stderrPresentationSink();
        }

        if (sink) {
            ++router.activePresentationDeliveries;
        }

        lock.unlock();

        if (sink) {
            s_inCallback = true;
            try {
                sink(rec.message);
            } catch (const std::exception& e) {
                std::fprintf(stderr, "qamui: presentation sink exception: %s\n", e.what());
                std::fflush(stderr);
            } catch (...) {
                std::fprintf(stderr, "qamui: presentation sink unknown exception\n");
                std::fflush(stderr);
            }
            s_inCallback = false;
        }

        lock.relock();

        if (sink) {
            --router.activePresentationDeliveries;
            if (router.regState == RegistrationState::Retiring
                && router.activePresentationDeliveries == 0
                && router.activeDiagnosticDeliveries == 0) {
                router.retirementWaitCondition.wakeAll();
            }
        }
    }
    router.isDraining = false;
}

void routeMessage(QtMsgType type, const QMessageLogContext& context,
                  const QString& message)
{
    if (s_inCallback) {
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
        if (context.category)
            msg.category = QByteArray(context.category);
        if (context.file)
            msg.file = QByteArray(context.file);
        if (context.function)
            msg.function = QByteArray(context.function);
        msg.line = context.line;
        msg.formattedText = qFormatLogMessage(type, context, message);

        QtMessageSink diagSink = nullptr;
        QtMessageSink presSink = nullptr;
        {
            QMutexLocker lock(&router.mutex);
            if (router.regState == RegistrationState::Active) {
                diagSink = router.diagnosticSink;
                if (router.hasExplicitPresentationSink)
                    presSink = router.presentationSink;
            } else {
                presSink = stderrPresentationSink();
            }
        }

        s_inCallback = true;
        if (diagSink) {
            try { diagSink(msg); } catch (...) {}
        }
        if (presSink) {
            try { presSink(msg); } catch (...) {}
        }
        std::fflush(stderr);
        s_inCallback = false;
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
        if (context.category)
            msg.category = QByteArray(context.category);
        if (context.file)
            msg.file = QByteArray(context.file);
        if (context.function)
            msg.function = QByteArray(context.function);
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
            constexpr int kMaxPresentationMessages = 500;
            if (router.alternateScreenSessionCount > 0
                && router.presentationQueue.size() >= kMaxPresentationMessages) {
                router.presentationQueue.removeFirst();
            }
            router.presentationQueue.append(PresentationRecord{
                seq,
                msg,
                false,
                regId
            });
        }

        if (diagSink) {
            ++router.activeDiagnosticDeliveries;
        }
    }

    if (diagSink) {
        s_inCallback = true;
        try {
            diagSink(msg);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "qamui: diagnostic sink exception: %s\n", e.what());
            std::fflush(stderr);
        } catch (...) {
            std::fprintf(stderr, "qamui: diagnostic sink unknown exception\n");
            std::fflush(stderr);
        }
        s_inCallback = false;
    }

    {
        QMutexLocker lock(&router.mutex);
        if (diagSink) {
            --router.activeDiagnosticDeliveries;
            if (router.regState == RegistrationState::Retiring
                && router.activeDiagnosticDeliveries == 0
                && router.activePresentationDeliveries == 0) {
                router.retirementWaitCondition.wakeAll();
            }
        }

        if (hasPresentation) {
            for (auto& rec : router.presentationQueue) {
                if (rec.seq == seq) {
                    rec.eligible = true;
                    break;
                }
            }
        }

        if (router.alternateScreenSessionCount == 0) {
            drainPresentationQueue(router, lock);
        }
    }
}

} // namespace

void registerEngine()
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    ++router.engineCount;
    if (!router.handlerInstalled) {
        router.previousHandler = qInstallMessageHandler(routeMessage);
        router.handlerInstalled = true;
    }
}

void unregisterEngine()
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.engineCount <= 0)
        return;

    --router.engineCount;
    if (router.engineCount == 0) {
        router.alternateScreenSessionCount = 0;
        drainPresentationQueue(router, lock);
        if (router.regState == RegistrationState::None && router.handlerInstalled) {
            qInstallMessageHandler(router.previousHandler);
            router.previousHandler = nullptr;
            router.handlerInstalled = false;
        }
    }
}

void beginAlternateScreenSession()
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    ++router.alternateScreenSessionCount;
}

void endAlternateScreenSession()
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.alternateScreenSessionCount <= 0)
        return;

    --router.alternateScreenSessionCount;
    if (router.alternateScreenSessionCount == 0) {
        drainPresentationQueue(router, lock);
    }
}

bool installRegistration(uint64_t registrationId,
                         QtMessageSink diagnosticSink,
                         QtMessageSink presentationSink,
                         bool hasPresentationSink)
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.regState != RegistrationState::None)
        return false;

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

void resetRegistration(uint64_t registrationId)
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    if (router.activeRegistrationId != registrationId
        || router.regState != RegistrationState::Active) {
        return;
    }

    router.regState = RegistrationState::Retiring;
    router.presentationQueue.clear();

    while (router.activeDiagnosticDeliveries > 0 || router.activePresentationDeliveries > 0) {
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

QtMessageSink stderrPresentationSink()
{
    return [](const QtMessage& msg) {
        RouterState& router = state();
        QtMessageHandler prev = nullptr;
        {
            QMutexLocker lock(&router.mutex);
            prev = router.previousHandler;
        }
        if (prev) {
            const QMessageLogContext context(
                msg.file.isNull() ? nullptr : msg.file.constData(),
                msg.line,
                msg.function.isNull() ? nullptr : msg.function.constData(),
                msg.category.isNull() ? nullptr : msg.category.constData());
            prev(msg.type, context, msg.message);
        } else {
            std::fprintf(stderr, "%s\n", qPrintable(msg.formattedMessage()));
            std::fflush(stderr);
        }
    };
}

} // namespace qamui::TerminalMessageRouter
