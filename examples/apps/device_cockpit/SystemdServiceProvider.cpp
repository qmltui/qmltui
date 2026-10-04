#include "SystemdServiceProvider.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QTimer>

SystemdServiceProvider::SystemdServiceProvider(QObject *parent)
    : ServiceProvider(5000, parent),
      m_manager(QStringLiteral("org.freedesktop.systemd1"),
                QStringLiteral("/org/freedesktop/systemd1"),
                QStringLiteral("org.freedesktop.systemd1.Manager"),
                QDBusConnection::systemBus()) {
    refresh();
}

void SystemdServiceProvider::refresh() {
    if (!m_manager.isValid()) {
        setCanManage(false);
        return;
    }

    QDBusPendingCall call = m_manager.asyncCall(QStringLiteral("ListUnits"));
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            setCanManage(false);
            return;
        }

        // ListUnits returns a(ssssssouso): name, description, load state,
        // active state, sub state, follower, object path, job id, job type,
        // job path. Only the first five matter here.
        QVariantList units;
        const QDBusArgument arg =
            watcher->reply().arguments().value(0).value<QDBusArgument>();
        arg.beginArray();
        while (!arg.atEnd()) {
            QString name, description, load, active, sub;
            arg.beginStructure();
            arg >> name >> description >> load >> active >> sub;
            // Consume the trailing fields so the structure stays aligned.
            QString follower;
            QVariant path, jobId, jobType, jobPath;
            arg >> follower >> path >> jobId >> jobType >> jobPath;
            arg.endStructure();
            if (name.endsWith(QStringLiteral(".service"))) {
                units.append(QVariantMap{{"unit", name},
                                         {"description", description},
                                         {"load", load},
                                         {"active", active},
                                         {"sub", sub}});
            }
        }
        arg.endArray();

        setCanManage(true);
        setAllUnits(std::move(units));
    });
}

void SystemdServiceProvider::startUnit(const QString &unit) {
    callUnitAction(unit, QStringLiteral("StartUnit"), QStringLiteral("start"));
}

void SystemdServiceProvider::stopUnit(const QString &unit) {
    callUnitAction(unit, QStringLiteral("StopUnit"), QStringLiteral("stop"));
}

void SystemdServiceProvider::restartUnit(const QString &unit) {
    callUnitAction(unit, QStringLiteral("RestartUnit"), QStringLiteral("restart"));
}

void SystemdServiceProvider::callUnitAction(const QString &unit,
                                            const QString &method,
                                            const QString &action) {
    QDBusPendingCall call =
        m_manager.asyncCall(method, unit, QStringLiteral("replace"));
    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, unit, action](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const bool ok = !watcher->isError();
        emit actionFinished(unit, action, ok,
                            ok ? QStringLiteral("%1: %2 requested").arg(unit, action)
                               : watcher->error().message());
        // Unit states settle shortly after the job is queued.
        QTimer::singleShot(600, this, &SystemdServiceProvider::refresh);
    });
}
