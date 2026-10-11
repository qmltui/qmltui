#pragma once

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QtQml/qqmlregistration.h>
#include <array>

class ProcessTableModel final : public QAbstractTableModel
{
public:
    explicit ProcessTableModel(QObject *parent = nullptr) : QAbstractTableModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    struct Process { int pid; const char *name; double cpu; const char *status; };
    const std::array<Process, 8> m_processes{{
        {1001, "systemd", 0.1, "RUNNING"},
        {1042, "terminal_engine", 12.4, "RUNNING"},
        {1089, "qml_renderer", 4.2, "RUNNING"},
        {1120, "dbus-daemon", 0.0, "STOPPED"},
        {1250, "net_stack", 1.8, "RUNNING"},
        {1304, "posix_tty", 0.5, "CRASHED"},
        {1410, "input_decoder", 0.2, "RUNNING"},
        {1520, "kworker/0:1", 0.0, "STOPPED"},
    }};
};

// Application policy, not a new framework proxy API. Qamui only consumes this model.
class ProcessProxy : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(QString sortLabel READ sortLabel NOTIFY sortLabelChanged FINAL)
public:
    explicit ProcessProxy(QObject *parent = nullptr);
    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);
    QString sortLabel() const;
    Q_INVOKABLE void headerClicked(int column);
    Q_INVOKABLE void activate(int proxyRow);
signals:
    void filterTextChanged();
    void sortLabelChanged();
    void activatedPid(int pid);
private:
    ProcessTableModel m_source;
    QString m_filterText;
};
