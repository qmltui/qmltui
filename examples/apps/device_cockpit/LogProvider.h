#pragma once

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QStringList>

// Live log stream domain. Lines are pre-formatted single-color text for a
// LogView; the page replays backlog() on creation and then appends from
// lineAppended(), so no lines are lost across page switches.
class LogProvider : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    explicit LogProvider(QObject *parent = nullptr);

    bool available() const { return m_available; }

    Q_INVOKABLE QStringList backlog() const { return m_backlog; }

signals:
    void lineAppended(const QString &line);
    void availableChanged();

protected:
    void appendLine(const QDateTime &timestamp, int priority,
                    const QString &source, const QString &message);
    void setAvailable(bool available);

private:
    static constexpr int kBacklogLimit = 500;

    QStringList m_backlog;
    bool m_available = false;
};
