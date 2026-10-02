#pragma once

#include <QAbstractItemModel>
#include <QElapsedTimer>
#include <QString>
#include <QtQml/qqmlregistration.h>
#include <QtTaskTree>

#include <memory>
#include <vector>

namespace qtui {

struct TaskNode
{
    QString id;
    QString name;
    QString type;
    QString command;
    QString status = QStringLiteral("Pending");
    QString logs;
    qint64 startedAtMs = -1;
    qint64 durationMs = 0;
    TaskNode *parent = nullptr;
    std::vector<std::unique_ptr<TaskNode>> children;
};

class TaskTreeModel final : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged FINAL)
    Q_PROPERTY(int progressValue READ progressValue NOTIFY progressValueChanged FINAL)
    Q_PROPERTY(int progressMaximum READ progressMaximum NOTIFY progressMaximumChanged FINAL)
    Q_PROPERTY(QString activeTaskLog READ activeTaskLog NOTIFY activeTaskLogChanged FINAL)
    Q_PROPERTY(QString selectedNodeId READ selectedNodeId NOTIFY selectedNodeIdChanged FINAL)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged FINAL)

public:
    enum Roles : int {
        StatusRole = Qt::UserRole + 1,
        StatusIconRole,
        StatusColorRole,
        DurationRole,
        LogsRole,
        TypeRole,
        CommandRole,
        NodeIdRole,
    };

    explicit TaskTreeModel(QObject *parent = nullptr);
    ~TaskTreeModel() override = default;

    bool isRunning() const { return m_isRunning; }
    int progressValue() const { return m_progressValue; }
    int progressMaximum() const { return m_progressMaximum; }
    QString activeTaskLog() const { return m_activeTaskLog; }
    QString selectedNodeId() const { return m_selectedNodeId; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE void startWorkflow();
    Q_INVOKABLE void cancelWorkflow();
    Q_INVOKABLE void resetWorkflow();
    Q_INVOKABLE void selectTask(const QString &nodeId);

    Q_INVOKABLE QString addTask(const QString &name, const QString &type,
                                const QString &parentId = QStringLiteral("root"),
                                const QString &command = {});
    Q_INVOKABLE bool removeTask(const QString &nodeId);
    Q_INVOKABLE void clearAllTasks();

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void isRunningChanged();
    void progressValueChanged();
    void progressMaximumChanged();
    void activeTaskLogChanged();
    void selectedNodeIdChanged();
    void lastErrorChanged();

private:
    void initializeSampleWorkflow();
    std::unique_ptr<TaskNode> makeNode(const QString &id, const QString &name,
                                       const QString &type, const QString &command = {}) const;
    TaskNode *nodeForIndex(const QModelIndex &index) const;
    QModelIndex indexForNode(const TaskNode *node) const;
    TaskNode *findNode(const QString &id) const;
    TaskNode *findNode(TaskNode *node, const QString &id) const;
    bool containsNode(const TaskNode *ancestor, const TaskNode *candidate) const;
    int rowOfNode(const TaskNode *node) const;
    int processTaskCount(const TaskNode *node) const;

    QtTaskTree::Group buildGroup(TaskNode *node);
    QtTaskTree::GroupItem buildItem(TaskNode *node);
    QtTaskTree::QProcessTask buildProcessTask(TaskNode *node);

    void setRunning(bool running);
    void setLastError(const QString &error);
    void setNodeStatus(TaskNode *node, const QString &status);
    void appendNodeLog(TaskNode *node, const QString &text);
    void resetRuntimeState(TaskNode *node);
    void finalizePendingNodes(TaskNode *node, QtTaskTree::DoneWith result);
    void updateProgressMaximumFromModel();
    void emitNodeChanged(TaskNode *node, const QList<int> &roles);
    void refreshActiveTaskLog();
    QString statusIcon(const QString &status) const;
    QString statusColor(const QString &status) const;

    std::unique_ptr<TaskNode> m_invisibleRoot;
    QtTaskTree::QTaskTree m_taskTree;
    QElapsedTimer m_runClock;
    bool m_isRunning = false;
    int m_progressValue = 0;
    int m_progressMaximum = 0;
    QString m_selectedNodeId = QStringLiteral("root");
    QString m_activeTaskLog;
    QString m_lastError;
};

} // namespace qtui
