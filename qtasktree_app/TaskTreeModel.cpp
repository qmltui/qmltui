#include "TaskTreeModel.h"

#include <QDir>
#include <QProcess>
#include <QUuid>

#include <algorithm>

namespace qtui {

using namespace QtTaskTree;

TaskTreeModel::TaskTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_taskTree(this)
{
    initializeSampleWorkflow();

    connect(&m_taskTree, &QTaskTree::started, this, [this] {
        setRunning(true);
    });
    connect(&m_taskTree, &QTaskTree::progressValueChanged, this, [this](qsizetype value) {
        const int newValue = static_cast<int>(value);
        if (m_progressValue == newValue)
            return;
        m_progressValue = newValue;
        emit progressValueChanged();
    });
    connect(&m_taskTree, &QTaskTree::done, this, [this](DoneWith result) {
        finalizePendingNodes(m_invisibleRoot.get(), result);
        setRunning(false);
        refreshActiveTaskLog();
    });
}

std::unique_ptr<TaskNode> TaskTreeModel::makeNode(const QString &id, const QString &name,
                                                  const QString &type,
                                                  const QString &command) const
{
    auto node = std::make_unique<TaskNode>();
    node->id = id;
    node->name = name;
    node->type = type;
    node->command = command;
    return node;
}

void TaskTreeModel::initializeSampleWorkflow()
{
    beginResetModel();

    m_invisibleRoot = makeNode(QStringLiteral("__invisible_root"), {}, QStringLiteral("Internal"));
    auto pipeline = makeNode(QStringLiteral("root"), QStringLiteral("Build and validation workflow"),
                             QStringLiteral("GroupSequential"));
    pipeline->logs = QStringLiteral("Ready to execute this workflow with Qt TaskTree.");

    auto build = makeNode(QStringLiteral("build"), QStringLiteral("Build qtasktree runner"),
                          QStringLiteral("QProcessTask"),
                          QStringLiteral("cmake --build build --target qtasktree_runner"));

    auto checks = makeNode(QStringLiteral("checks"), QStringLiteral("Parallel validation"),
                           QStringLiteral("GroupParallel"));
    auto tests = makeNode(QStringLiteral("tests"), QStringLiteral("Run repository tests"),
                          QStringLiteral("QProcessTask"),
                          QStringLiteral("ctest --test-dir build --output-on-failure"));
    auto status = makeNode(QStringLiteral("status"), QStringLiteral("Inspect working tree"),
                           QStringLiteral("QProcessTask"), QStringLiteral("git status --short"));

    tests->parent = checks.get();
    status->parent = checks.get();
    checks->children.push_back(std::move(tests));
    checks->children.push_back(std::move(status));

    build->parent = pipeline.get();
    checks->parent = pipeline.get();
    pipeline->children.push_back(std::move(build));
    pipeline->children.push_back(std::move(checks));

    pipeline->parent = m_invisibleRoot.get();
    m_invisibleRoot->children.push_back(std::move(pipeline));

    endResetModel();
    resetWorkflow();
}

TaskNode *TaskTreeModel::nodeForIndex(const QModelIndex &index) const
{
    if (!index.isValid())
        return m_invisibleRoot.get();
    return static_cast<TaskNode *>(index.internalPointer());
}

int TaskTreeModel::rowOfNode(const TaskNode *node) const
{
    if (!node || !node->parent)
        return -1;

    const auto &siblings = node->parent->children;
    const auto it = std::find_if(siblings.cbegin(), siblings.cend(),
                                 [node](const auto &candidate) {
                                     return candidate.get() == node;
                                 });
    return it == siblings.cend() ? -1 : static_cast<int>(std::distance(siblings.cbegin(), it));
}

QModelIndex TaskTreeModel::indexForNode(const TaskNode *node) const
{
    if (!node || node == m_invisibleRoot.get())
        return {};
    const int row = rowOfNode(node);
    return row < 0 ? QModelIndex() : createIndex(row, 0, const_cast<TaskNode *>(node));
}

TaskNode *TaskTreeModel::findNode(TaskNode *node, const QString &id) const
{
    if (!node)
        return nullptr;
    if (node->id == id)
        return node;
    for (const auto &child : node->children) {
        if (TaskNode *match = findNode(child.get(), id))
            return match;
    }
    return nullptr;
}

TaskNode *TaskTreeModel::findNode(const QString &id) const
{
    return findNode(m_invisibleRoot.get(), id);
}

bool TaskTreeModel::containsNode(const TaskNode *ancestor, const TaskNode *candidate) const
{
    if (!ancestor || !candidate)
        return false;
    if (ancestor == candidate)
        return true;
    for (const auto &child : ancestor->children) {
        if (containsNode(child.get(), candidate))
            return true;
    }
    return false;
}

int TaskTreeModel::processTaskCount(const TaskNode *node) const
{
    if (!node)
        return 0;
    int count = node->type == QStringLiteral("QProcessTask") ? 1 : 0;
    for (const auto &child : node->children)
        count += processTaskCount(child.get());
    return count;
}

QModelIndex TaskTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0 || row < 0)
        return {};

    TaskNode *parentNode = nodeForIndex(parent);
    if (!parentNode || row >= static_cast<int>(parentNode->children.size()))
        return {};
    return createIndex(row, column, parentNode->children[static_cast<size_t>(row)].get());
}

QModelIndex TaskTreeModel::parent(const QModelIndex &child) const
{
    TaskNode *node = nodeForIndex(child);
    if (!node || !node->parent || node->parent == m_invisibleRoot.get())
        return {};
    return indexForNode(node->parent);
}

int TaskTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    const TaskNode *parentNode = nodeForIndex(parent);
    return parentNode ? static_cast<int>(parentNode->children.size()) : 0;
}

int TaskTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant TaskTreeModel::data(const QModelIndex &index, int role) const
{
    const TaskNode *node = nodeForIndex(index);
    if (!index.isValid() || !node || node == m_invisibleRoot.get())
        return {};

    if (role == Qt::DisplayRole)
        return node->name;
    if (role == StatusRole)
        return node->status;
    if (role == StatusIconRole)
        return statusIcon(node->status);
    if (role == StatusColorRole)
        return statusColor(node->status);
    if (role == DurationRole)
        return node->durationMs > 0
            ? QString::number(node->durationMs / 1000.0, 'f', 2) + QStringLiteral("s")
            : QString();
    if (role == LogsRole)
        return node->logs;
    if (role == TypeRole)
        return node->type;
    if (role == CommandRole)
        return node->command;
    if (role == NodeIdRole)
        return node->id;
    return {};
}

QHash<int, QByteArray> TaskTreeModel::roleNames() const
{
    static const QHash<int, QByteArray> roles {
        {Qt::DisplayRole, "display"},
        {StatusRole, "status"},
        {StatusIconRole, "statusIcon"},
        {StatusColorRole, "statusColor"},
        {DurationRole, "duration"},
        {LogsRole, "logs"},
        {TypeRole, "type"},
        {CommandRole, "command"},
        {NodeIdRole, "nodeId"},
    };
    return roles;
}

QString TaskTreeModel::addTask(const QString &name, const QString &type,
                               const QString &parentId, const QString &command)
{
    if (m_isRunning) {
        setLastError(QStringLiteral("Stop the workflow before editing it."));
        return {};
    }

    TaskNode *parentNode = findNode(parentId);
    if (!parentNode)
        parentNode = findNode(QStringLiteral("root"));
    if (!parentNode || !parentNode->type.startsWith(QStringLiteral("Group"))) {
        setLastError(QStringLiteral("New tasks can only be added below a group."));
        return {};
    }

    static const QStringList validTypes {
        QStringLiteral("QProcessTask"),
        QStringLiteral("GroupSequential"),
        QStringLiteral("GroupParallel"),
    };
    const QString normalizedType = validTypes.contains(type) ? type : QStringLiteral("QProcessTask");
    const QString normalizedName = name.trimmed();
    if (normalizedName.isEmpty()) {
        setLastError(QStringLiteral("Task name cannot be empty."));
        return {};
    }
    if (normalizedType == QStringLiteral("QProcessTask") && command.trimmed().isEmpty()) {
        setLastError(QStringLiteral("A process task requires a command."));
        return {};
    }

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto node = makeNode(id, normalizedName, normalizedType, command.trimmed());
    node->parent = parentNode;

    const QModelIndex parentIndex = indexForNode(parentNode);
    const int row = static_cast<int>(parentNode->children.size());
    beginInsertRows(parentIndex, row, row);
    parentNode->children.push_back(std::move(node));
    endInsertRows();

    updateProgressMaximumFromModel();
    setLastError({});
    selectTask(id);
    return id;
}

bool TaskTreeModel::removeTask(const QString &nodeId)
{
    if (m_isRunning) {
        setLastError(QStringLiteral("Stop the workflow before editing it."));
        return false;
    }

    TaskNode *node = findNode(nodeId);
    if (!node || node->id == QStringLiteral("root") || !node->parent) {
        setLastError(QStringLiteral("The workflow root cannot be removed."));
        return false;
    }

    TaskNode *selected = findNode(m_selectedNodeId);
    const bool removesSelection = containsNode(node, selected);
    TaskNode *parentNode = node->parent;
    const int row = rowOfNode(node);
    if (row < 0)
        return false;

    beginRemoveRows(indexForNode(parentNode), row, row);
    parentNode->children.erase(parentNode->children.begin() + row);
    endRemoveRows();

    if (removesSelection)
        selectTask(parentNode->id);
    updateProgressMaximumFromModel();
    setLastError({});
    return true;
}

void TaskTreeModel::clearAllTasks()
{
    if (m_isRunning) {
        setLastError(QStringLiteral("Stop the workflow before editing it."));
        return;
    }

    TaskNode *root = findNode(QStringLiteral("root"));
    if (!root || root->children.empty())
        return;

    const int lastRow = static_cast<int>(root->children.size()) - 1;
    beginRemoveRows(indexForNode(root), 0, lastRow);
    root->children.clear();
    endRemoveRows();
    selectTask(root->id);
    updateProgressMaximumFromModel();
}

void TaskTreeModel::selectTask(const QString &nodeId)
{
    TaskNode *node = findNode(nodeId);
    if (!node || node == m_invisibleRoot.get())
        return;

    if (m_selectedNodeId != nodeId) {
        m_selectedNodeId = nodeId;
        emit selectedNodeIdChanged();
    }
    refreshActiveTaskLog();
}

void TaskTreeModel::setRunning(bool running)
{
    if (m_isRunning == running)
        return;
    m_isRunning = running;
    emit isRunningChanged();
}

void TaskTreeModel::setLastError(const QString &error)
{
    if (m_lastError == error)
        return;
    m_lastError = error;
    emit lastErrorChanged();
}

void TaskTreeModel::emitNodeChanged(TaskNode *node, const QList<int> &roles)
{
    const QModelIndex changedIndex = indexForNode(node);
    if (changedIndex.isValid())
        emit dataChanged(changedIndex, changedIndex, roles);
}

void TaskTreeModel::setNodeStatus(TaskNode *node, const QString &status)
{
    if (!node || node->status == status)
        return;
    node->status = status;
    emitNodeChanged(node, {StatusRole, StatusIconRole, StatusColorRole, DurationRole});
    if (node->id == m_selectedNodeId)
        refreshActiveTaskLog();
}

void TaskTreeModel::appendNodeLog(TaskNode *node, const QString &text)
{
    if (!node || text.isEmpty())
        return;
    node->logs += text;
    emitNodeChanged(node, {LogsRole});
    if (node->id == m_selectedNodeId)
        refreshActiveTaskLog();
}

void TaskTreeModel::resetRuntimeState(TaskNode *node)
{
    if (!node)
        return;
    if (node != m_invisibleRoot.get()) {
        node->status = QStringLiteral("Pending");
        node->durationMs = 0;
        node->startedAtMs = -1;
        if (node->type == QStringLiteral("QProcessTask"))
            node->logs.clear();
        emitNodeChanged(node, {StatusRole, StatusIconRole, StatusColorRole, DurationRole, LogsRole});
    }
    for (const auto &child : node->children)
        resetRuntimeState(child.get());
}

void TaskTreeModel::resetWorkflow()
{
    if (m_taskTree.isRunning()) {
        setLastError(QStringLiteral("Stop the workflow before resetting its status."));
        return;
    }

    resetRuntimeState(m_invisibleRoot.get());
    m_progressValue = 0;
    emit progressValueChanged();
    updateProgressMaximumFromModel();
    refreshActiveTaskLog();
    setLastError({});
}

void TaskTreeModel::updateProgressMaximumFromModel()
{
    const int maximum = processTaskCount(findNode(QStringLiteral("root")));
    if (m_progressMaximum == maximum)
        return;
    m_progressMaximum = maximum;
    emit progressMaximumChanged();
}

QProcessTask TaskTreeModel::buildProcessTask(TaskNode *node)
{
    const QString nodeId = node->id;
    const auto setup = [this, nodeId](QProcess &process) -> SetupResult {
        TaskNode *current = findNode(nodeId);
        if (!current)
            return SetupResult::StopWithError;

        QStringList commandParts = QProcess::splitCommand(current->command);
        if (commandParts.isEmpty()) {
            setNodeStatus(current, QStringLiteral("Failed"));
            appendNodeLog(current, QStringLiteral("Command is empty.\n"));
            return SetupResult::StopWithError;
        }

        const QString program = commandParts.takeFirst();
        current->startedAtMs = m_runClock.elapsed();
        setNodeStatus(current, QStringLiteral("Running"));
        appendNodeLog(current, QStringLiteral("$ %1\n").arg(current->command));

        process.setProgram(program);
        process.setArguments(commandParts);
        process.setWorkingDirectory(QDir::currentPath());
        process.setProcessChannelMode(QProcess::SeparateChannels);

        QProcess *processPointer = &process;
        connect(&process, &QProcess::readyReadStandardOutput, this,
                [this, nodeId, processPointer] {
                    appendNodeLog(findNode(nodeId),
                                  QString::fromLocal8Bit(processPointer->readAllStandardOutput()));
                });
        connect(&process, &QProcess::readyReadStandardError, this,
                [this, nodeId, processPointer] {
                    appendNodeLog(findNode(nodeId),
                                  QString::fromLocal8Bit(processPointer->readAllStandardError()));
                });
        return SetupResult::Continue;
    };

    const auto done = [this, nodeId](const QProcess &process, DoneWith result) {
        TaskNode *current = findNode(nodeId);
        if (!current)
            return;

        if (current->startedAtMs >= 0)
            current->durationMs = m_runClock.elapsed() - current->startedAtMs;

        if (result == DoneWith::Success) {
            setNodeStatus(current, QStringLiteral("Success"));
        } else if (result == DoneWith::Cancel) {
            setNodeStatus(current, QStringLiteral("Canceled"));
        } else {
            setNodeStatus(current, QStringLiteral("Failed"));
            appendNodeLog(current, QStringLiteral("\nProcess failed with exit code %1.\n")
                                       .arg(process.exitCode()));
        }
        emitNodeChanged(current, {DurationRole});
    };

    return QProcessTask(setup, done);
}

GroupItem TaskTreeModel::buildItem(TaskNode *node)
{
    if (node->type.startsWith(QStringLiteral("Group")))
        return buildGroup(node);
    return buildProcessTask(node);
}

Group TaskTreeModel::buildGroup(TaskNode *node)
{
    const QString nodeId = node->id;
    GroupItems items;
    items << (node->type == QStringLiteral("GroupParallel") ? parallel : sequential);
    items << stopOnError;
    items << Group::onGroupSetup([this, nodeId] {
        TaskNode *current = findNode(nodeId);
        if (!current)
            return SetupResult::StopWithError;
        current->startedAtMs = m_runClock.elapsed();
        setNodeStatus(current, QStringLiteral("Running"));
        return SetupResult::Continue;
    });
    items << Group::onGroupDone([this, nodeId](DoneWith result) {
        TaskNode *current = findNode(nodeId);
        if (!current)
            return;
        if (current->startedAtMs >= 0)
            current->durationMs = m_runClock.elapsed() - current->startedAtMs;
        if (result == DoneWith::Success)
            setNodeStatus(current, QStringLiteral("Success"));
        else if (result == DoneWith::Cancel)
            setNodeStatus(current, QStringLiteral("Canceled"));
        else
            setNodeStatus(current, QStringLiteral("Failed"));
        emitNodeChanged(current, {DurationRole});
    });

    for (const auto &child : node->children)
        items << buildItem(child.get());
    return Group(items);
}

void TaskTreeModel::startWorkflow()
{
    if (m_taskTree.isRunning())
        return;

    TaskNode *root = findNode(QStringLiteral("root"));
    if (!root || processTaskCount(root) == 0) {
        setLastError(QStringLiteral("Add at least one process task before running."));
        return;
    }

    resetRuntimeState(m_invisibleRoot.get());
    setLastError({});
    m_progressValue = 0;
    emit progressValueChanged();
    m_runClock.restart();

    m_taskTree.setRecipe(buildGroup(root));
    const int maximum = static_cast<int>(m_taskTree.progressMaximum());
    if (m_progressMaximum != maximum) {
        m_progressMaximum = maximum;
        emit progressMaximumChanged();
    }
    m_taskTree.start();
}

void TaskTreeModel::cancelWorkflow()
{
    if (m_taskTree.isRunning())
        m_taskTree.cancel();
}

void TaskTreeModel::finalizePendingNodes(TaskNode *node, DoneWith result)
{
    if (!node)
        return;
    if (node != m_invisibleRoot.get() && node->status == QStringLiteral("Pending")) {
        setNodeStatus(node, result == DoneWith::Cancel
                                ? QStringLiteral("Canceled")
                                : QStringLiteral("Skipped"));
    }
    for (const auto &child : node->children)
        finalizePendingNodes(child.get(), result);
}

void TaskTreeModel::refreshActiveTaskLog()
{
    const TaskNode *node = findNode(m_selectedNodeId);
    const QString text = node
        ? QStringLiteral("%1\nType: %2\nStatus: %3\nCommand: %4\n----------------------------------------\n%5")
              .arg(node->name, node->type, node->status,
                   node->command.isEmpty() ? QStringLiteral("-") : node->command,
                   node->logs.isEmpty() ? QStringLiteral("No output.") : node->logs)
        : QStringLiteral("No task selected.");
    if (m_activeTaskLog == text)
        return;
    m_activeTaskLog = text;
    emit activeTaskLogChanged();
}

QString TaskTreeModel::statusIcon(const QString &status) const
{
    if (status == QStringLiteral("Running"))
        return QStringLiteral("[..]");
    if (status == QStringLiteral("Success"))
        return QStringLiteral("[OK]");
    if (status == QStringLiteral("Failed"))
        return QStringLiteral("[!!]");
    if (status == QStringLiteral("Canceled"))
        return QStringLiteral("[XX]");
    if (status == QStringLiteral("Skipped"))
        return QStringLiteral("[--]");
    return QStringLiteral("[  ]");
}

QString TaskTreeModel::statusColor(const QString &status) const
{
    if (status == QStringLiteral("Running"))
        return QStringLiteral("yellow");
    if (status == QStringLiteral("Success"))
        return QStringLiteral("green");
    if (status == QStringLiteral("Failed"))
        return QStringLiteral("red");
    if (status == QStringLiteral("Canceled"))
        return QStringLiteral("magenta");
    return QStringLiteral("gray");
}

} // namespace qtui
