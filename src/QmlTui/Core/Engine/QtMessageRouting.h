#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlogging.h>
#include <QtCore/qstring.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace qmltui {

// Captured Qt log message delivered to a QtMessageSink.
struct QMLTUI_CORE_EXPORT QtMessage {
    QtMsgType type = QtDebugMsg;
    QString message;
    QDateTime timestampUtc;
    QByteArray category;
    QByteArray file;
    QByteArray function;
    int line = 0;
    QString formattedText;

    const QString &formattedMessage() const noexcept { return formattedText; }
};

using QtMessageSink = std::function<void(const QtMessage &)>;

// RAII registration routing qDebug/qInfo/qWarning/qCritical messages through
// application-supplied sinks while a TerminalEngine is active.
//
// While an alternate-screen session owns the terminal, presentation output to
// stderr is suppressed so log lines do not overwrite the TUI frame; diagnosticSink
// still receives every message. Destroying the returned handle or calling
// reset() restores the previous message handler.
class QMLTUI_CORE_EXPORT QtMessageRouting {
public:
    QtMessageRouting(const QtMessageRouting &) = delete;
    QtMessageRouting &operator=(const QtMessageRouting &) = delete;
    QtMessageRouting(QtMessageRouting &&) = delete;
    QtMessageRouting &operator=(QtMessageRouting &&) = delete;
    ~QtMessageRouting();

    // Installs diagnosticSink (always called) and an optional presentationSink
    // (called only when no alternate-screen session is active).
    [[nodiscard]] static std::unique_ptr<QtMessageRouting> install(
        QtMessageSink diagnosticSink,
        QtMessageSink presentationSink = {});

    // Default presentation sink that writes formattedText to stderr.
    static QtMessageSink stderrPresentationSink();

    // Unregisters this routing handle early; idempotent.
    void reset();

    struct Token { explicit Token() = default; };
    explicit QtMessageRouting(Token, uint64_t registrationId);

private:
    bool m_active{true};
    uint64_t m_registrationId{0};
};

} // namespace qmltui
