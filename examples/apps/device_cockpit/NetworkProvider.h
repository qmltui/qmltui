#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QVariantList>

// Network configuration domain. Interface entries are maps with the keys
// { name, up, method, ipv4, prefix, gateway, mac }; method is "dhcp",
// "static", or "—" when unknown.
class NetworkProvider : public QObject {
    Q_OBJECT
    QML_ANONYMOUS

    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY interfacesChanged)
    Q_PROPERTY(bool canConfigure READ canConfigure NOTIFY canConfigureChanged)

public:
    explicit NetworkProvider(int refreshIntervalMs, QObject *parent = nullptr);

    QVariantList interfaces() const { return m_interfaces; }

    /// False when the backend can only observe (no NetworkManager); the edit
    /// action is hidden.
    bool canConfigure() const { return m_canConfigure; }

    Q_INVOKABLE virtual void applyIpv4(const QString &name, const QString &method,
                                       const QString &address, int prefix,
                                       const QString &gateway) = 0;

signals:
    void interfacesChanged();
    void canConfigureChanged();
    void applyFinished(const QString &name, bool ok, const QString &message);

protected slots:
    virtual void refresh() = 0;

protected:
    void setInterfaces(QVariantList interfaces);
    void setCanConfigure(bool canConfigure);

    QVariantList m_interfaces;

private:
    bool m_canConfigure = false;
    QTimer m_timer;
};
