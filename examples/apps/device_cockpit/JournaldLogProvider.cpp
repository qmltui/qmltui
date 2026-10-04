#include "JournaldLogProvider.h"

#include <QJsonDocument>
#include <QJsonObject>

JournaldLogProvider::JournaldLogProvider(QObject *parent) : LogProvider(parent) {
    connect(&m_journalctl, &QProcess::finished, this,
            &JournaldLogProvider::handleFinished);
    connect(&m_journalctl, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) { setAvailable(false); });

    connect(&m_timer, &QTimer::timeout, this, &JournaldLogProvider::poll);
    m_timer.start(2000);
    poll();
}

void JournaldLogProvider::poll() {
    if (m_journalctl.state() != QProcess::NotRunning)
        return;

    QStringList arguments{QStringLiteral("-o"), QStringLiteral("json"),
                          QStringLiteral("--show-cursor")};
    if (m_cursor.isEmpty())
        arguments << QStringLiteral("-n") << QStringLiteral("40");
    else
        arguments << QStringLiteral("--after-cursor") << m_cursor;

    m_journalctl.start(QStringLiteral("journalctl"), arguments);
}

void JournaldLogProvider::handleFinished(int exitCode, QProcess::ExitStatus status) {
    if (status != QProcess::NormalExit || exitCode != 0) {
        setAvailable(false);
        return;
    }
    setAvailable(true);

    while (m_journalctl.canReadLine()) {
        const QByteArray raw = m_journalctl.readLine().trimmed();
        if (raw.isEmpty())
            continue;
        if (raw.startsWith("-- cursor: ")) {
            m_cursor = QString::fromUtf8(raw.mid(11));
            continue;
        }

        const QJsonObject entry = QJsonDocument::fromJson(raw).object();
        if (entry.isEmpty())
            continue;

        // MESSAGE can be a byte array (list of ints) for binary payloads.
        QString message = entry.value(QStringLiteral("MESSAGE")).toString();
        if (message.isEmpty() && entry.contains(QStringLiteral("MESSAGE")))
            message = QStringLiteral("[binary payload]");

        QString source = entry.value(QStringLiteral("_SYSTEMD_UNIT")).toString();
        if (source.isEmpty())
            source = entry.value(QStringLiteral("SYSLOG_IDENTIFIER")).toString();
        if (source.isEmpty())
            source = entry.value(QStringLiteral("_EXE")).toString().section('/', -1);
        if (source.isEmpty())
            source = QStringLiteral("journal");

        const qint64 microseconds =
            entry.value(QStringLiteral("__REALTIME_TIMESTAMP")).toString().toLongLong();
        const QDateTime timestamp = microseconds > 0
            ? QDateTime::fromMSecsSinceEpoch(microseconds / 1000)
            : QDateTime::currentDateTime();

        bool priorityOk = false;
        int priority =
            entry.value(QStringLiteral("PRIORITY")).toString().toInt(&priorityOk);
        if (!priorityOk)
            priority = 6;

        appendLine(timestamp, priority, source, message);
    }
}
