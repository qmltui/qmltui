#include <Qtui/Core/engine/TerminalMessageRouter_p.h>

#include <QByteArray>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QtLogging>

#include <cstdio>
#include <utility>

namespace qtui::TerminalMessageRouter {
namespace {

struct BufferedMessage
{
    QtMsgType type;
    QString message;
    QByteArray file;
    int line;
    QByteArray function;
    QByteArray category;
};

struct RouterState
{
    QMutex mutex;
    int engineCount = 0;
    int alternateScreenSessionCount = 0;
    QtMessageHandler previousHandler = nullptr;
    QList<BufferedMessage> messages;
};

RouterState& state()
{
    // The Qt message handler is process-global and may be called during static
    // destruction. Keep its state alive until process termination.
    static RouterState* routerState = new RouterState;
    return *routerState;
}

void deliver(QtMessageHandler handler, QtMsgType type,
             const QMessageLogContext& context, const QString& message)
{
    if (handler)
        handler(type, context, message);
    else {
        const QString formatted = qFormatLogMessage(type, context, message);
        std::fprintf(stderr, "%s\n", qPrintable(formatted));
        std::fflush(stderr);
    }
}

void deliver(QtMessageHandler handler, const BufferedMessage& buffered)
{
    const QMessageLogContext context(
        buffered.file.isNull() ? nullptr : buffered.file.constData(),
        buffered.line,
        buffered.function.isNull() ? nullptr : buffered.function.constData(),
        buffered.category.isNull() ? nullptr : buffered.category.constData());
    deliver(handler, buffered.type, context, buffered.message);
}

void flushMessages(QtMessageHandler handler, QList<BufferedMessage> messages)
{
    for (const BufferedMessage& message : messages)
        deliver(handler, message);
}

void routeMessage(QtMsgType type, const QMessageLogContext& context,
                  const QString& message)
{
    if (type == QtWarningMsg
        && message.contains(QLatin1String("Setting initial properties failed"))) {
        return;
    }

    RouterState& router = state();
    QtMessageHandler previousHandler = nullptr;
    {
        QMutexLocker lock(&router.mutex);
        previousHandler = router.previousHandler;
        if (router.alternateScreenSessionCount > 0 && type != QtFatalMsg) {
            router.messages.append(BufferedMessage{
                type,
                message,
                context.file ? QByteArray(context.file) : QByteArray(),
                context.line,
                context.function ? QByteArray(context.function) : QByteArray(),
                context.category ? QByteArray(context.category) : QByteArray()
            });
            return;
        }
    }

    // Fatal messages cannot safely be deferred because Qt aborts immediately
    // after the handler returns.
    deliver(previousHandler, type, context, message);
}

} // namespace

void registerEngine()
{
    RouterState& router = state();
    QMutexLocker lock(&router.mutex);
    ++router.engineCount;
    if (router.engineCount == 1)
        router.previousHandler = qInstallMessageHandler(routeMessage);
}

void unregisterEngine()
{
    RouterState& router = state();
    QtMessageHandler previousHandler = nullptr;
    QList<BufferedMessage> messages;
    {
        QMutexLocker lock(&router.mutex);
        if (router.engineCount <= 0)
            return;

        --router.engineCount;
        if (router.engineCount != 0)
            return;

        // TerminalEngine ends its session before unregistering. This fallback
        // prevents a broken lifecycle from leaving the global handler installed.
        router.alternateScreenSessionCount = 0;
        messages.swap(router.messages);
        previousHandler = router.previousHandler;
        router.previousHandler = nullptr;
        qInstallMessageHandler(previousHandler);
    }

    flushMessages(previousHandler, std::move(messages));
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
    QtMessageHandler previousHandler = nullptr;
    QList<BufferedMessage> messages;
    {
        QMutexLocker lock(&router.mutex);
        if (router.alternateScreenSessionCount <= 0)
            return;

        --router.alternateScreenSessionCount;
        if (router.alternateScreenSessionCount != 0)
            return;

        previousHandler = router.previousHandler;
        messages.swap(router.messages);
    }

    flushMessages(previousHandler, std::move(messages));
}

} // namespace qtui::TerminalMessageRouter
