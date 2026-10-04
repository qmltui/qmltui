#include "LogProvider.h"

LogProvider::LogProvider(QObject *parent) : QObject(parent) {}

void LogProvider::appendLine(const QDateTime &timestamp, int priority,
                             const QString &source, const QString &message) {
    QString tag = QStringLiteral("   ");
    if (priority <= 3)
        tag = QStringLiteral("ERR");
    else if (priority == 4)
        tag = QStringLiteral("WRN");

    const QString line = QStringLiteral("%1 %2 %3: %4")
                             .arg(timestamp.toString(QStringLiteral("hh:mm:ss")),
                                  tag, source, message);
    m_backlog.append(line);
    while (m_backlog.size() > kBacklogLimit)
        m_backlog.removeFirst();
    emit lineAppended(line);
}

void LogProvider::setAvailable(bool available) {
    if (m_available == available)
        return;
    m_available = available;
    emit availableChanged();
}
