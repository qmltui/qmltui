#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qlogging.h>
#include <QtCore/qstring.h>

#include <cstdint>
#include <functional>
#include <memory>

namespace qamui {

struct QAMUI_CORE_EXPORT QtMessage {
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

class QAMUI_CORE_EXPORT QtMessageRouting {
public:
    QtMessageRouting(const QtMessageRouting &) = delete;
    QtMessageRouting &operator=(const QtMessageRouting &) = delete;
    QtMessageRouting(QtMessageRouting &&) = delete;
    QtMessageRouting &operator=(QtMessageRouting &&) = delete;
    ~QtMessageRouting();

    [[nodiscard]] static std::unique_ptr<QtMessageRouting> install(
        QtMessageSink diagnosticSink,
        QtMessageSink presentationSink = {});

    static QtMessageSink stderrPresentationSink();

    void reset();

    struct Token { explicit Token() = default; };
    explicit QtMessageRouting(Token, uint64_t registrationId);

private:
    bool m_active{true};
    uint64_t m_registrationId{0};
};

} // namespace qamui
