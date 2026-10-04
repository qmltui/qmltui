#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QTimer>
#include <QVariantList>

// Service management domain: a filterable list of service units plus
// start/stop/restart actions. Unit entries are maps with the keys
// { unit, description, load, active, sub }.
class ServiceProvider : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QVariantList units READ units NOTIFY unitsChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(bool canManage READ canManage NOTIFY canManageChanged)

public:
    explicit ServiceProvider(int refreshIntervalMs, QObject *parent = nullptr);

    /// Units matching the current filter, sorted by unit name.
    QVariantList units() const;

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);

    /// False while the service manager is unreachable; actions are hidden.
    bool canManage() const { return m_canManage; }

    Q_INVOKABLE virtual void startUnit(const QString &unit) = 0;
    Q_INVOKABLE virtual void stopUnit(const QString &unit) = 0;
    Q_INVOKABLE virtual void restartUnit(const QString &unit) = 0;

signals:
    void unitsChanged();
    void filterChanged();
    void canManageChanged();
    void actionFinished(const QString &unit, const QString &action, bool ok,
                        const QString &message);

protected slots:
    virtual void refresh() = 0;

protected:
    void setAllUnits(QVariantList units);
    void setCanManage(bool canManage);

    QVariantList m_allUnits;

private:
    QString m_filter;
    bool m_canManage = false;
    QTimer m_timer;
};
