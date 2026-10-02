#include <QAbstractItemModelTester>
#include <QSignalSpy>
#include <QTest>

#include <TaskTreeModel.h>

using qtui::TaskTreeModel;

class TestTaskTreeModel : public QObject
{
    Q_OBJECT

private:
    static QModelIndex findById(const TaskTreeModel &model, const QString &id,
                                const QModelIndex &parent = {})
    {
        for (int row = 0; row < model.rowCount(parent); ++row) {
            const QModelIndex current = model.index(row, 0, parent);
            if (model.data(current, TaskTreeModel::NodeIdRole).toString() == id)
                return current;
            const QModelIndex nested = findById(model, id, current);
            if (nested.isValid())
                return nested;
        }
        return {};
    }

private slots:
    void modelContractAndRecursiveRemoval()
    {
        TaskTreeModel model;
        QAbstractItemModelTester tester(&model,
            QAbstractItemModelTester::FailureReportingMode::QtTest);

        QCOMPARE(model.rowCount(), 1);
        const QModelIndex root = findById(model, QStringLiteral("root"));
        QVERIFY(root.isValid());
        QCOMPARE(model.rowCount(root), 2);

        const QString childId = model.addTask(QStringLiteral("Nested task"),
                                               QStringLiteral("QProcessTask"),
                                               QStringLiteral("checks"),
                                               QStringLiteral("/bin/true"));
        QVERIFY(!childId.isEmpty());
        QVERIFY(findById(model, childId).isValid());

        QVERIFY(model.removeTask(QStringLiteral("checks")));
        QVERIFY(!findById(model, QStringLiteral("checks")).isValid());
        QVERIFY(!findById(model, childId).isValid());
        QCOMPARE(model.rowCount(root), 1);
    }

    void executesRealProcessAndPreservesRecipe()
    {
        TaskTreeModel model;
        QAbstractItemModelTester tester(&model,
            QAbstractItemModelTester::FailureReportingMode::QtTest);
        model.clearAllTasks();

        const QString taskId = model.addTask(QStringLiteral("Echo"),
                                             QStringLiteral("QProcessTask"),
                                             QStringLiteral("root"),
                                             QStringLiteral("/bin/sh -c \"printf functional\""));
        QVERIFY(!taskId.isEmpty());
        QCOMPARE(model.progressMaximum(), 1);

        model.startWorkflow();
        QTRY_VERIFY_WITH_TIMEOUT(!model.isRunning(), 5000);
        QCOMPARE(model.progressMaximum(), 1);
        QCOMPARE(model.progressValue(), 1);

        const QModelIndex task = findById(model, taskId);
        QVERIFY(task.isValid());
        QCOMPARE(model.data(task, TaskTreeModel::StatusRole).toString(),
                 QStringLiteral("Success"));
        QVERIFY(model.data(task, TaskTreeModel::LogsRole).toString()
                    .contains(QStringLiteral("functional")));
        QVERIFY(findById(model, taskId).isValid());
    }

    void reportsProcessFailureAndSkipsFollowingTask()
    {
        TaskTreeModel model;
        model.clearAllTasks();

        const QString failingId = model.addTask(QStringLiteral("Fail"),
                                                QStringLiteral("QProcessTask"),
                                                QStringLiteral("root"),
                                                QStringLiteral("/bin/sh -c \"exit 7\""));
        const QString skippedId = model.addTask(QStringLiteral("Skipped"),
                                                QStringLiteral("QProcessTask"),
                                                QStringLiteral("root"),
                                                QStringLiteral("/bin/true"));

        model.startWorkflow();
        QTRY_VERIFY_WITH_TIMEOUT(!model.isRunning(), 5000);

        const QModelIndex failing = findById(model, failingId);
        const QModelIndex skipped = findById(model, skippedId);
        QCOMPARE(model.data(failing, TaskTreeModel::StatusRole).toString(),
                 QStringLiteral("Failed"));
        QCOMPARE(model.data(skipped, TaskTreeModel::StatusRole).toString(),
                 QStringLiteral("Skipped"));
        QCOMPARE(model.progressValue(), model.progressMaximum());
    }

    void cancelsRunningProcess()
    {
        TaskTreeModel model;
        model.clearAllTasks();

        const QString taskId = model.addTask(QStringLiteral("Wait"),
                                             QStringLiteral("QProcessTask"),
                                             QStringLiteral("root"),
                                             QStringLiteral("/bin/sh -c \"sleep 5\""));
        model.startWorkflow();
        QVERIFY(model.isRunning());
        model.cancelWorkflow();
        QTRY_VERIFY_WITH_TIMEOUT(!model.isRunning(), 5000);

        const QModelIndex task = findById(model, taskId);
        QCOMPARE(model.data(task, TaskTreeModel::StatusRole).toString(),
                 QStringLiteral("Canceled"));
        QCOMPARE(model.progressValue(), model.progressMaximum());
    }
};

QTEST_MAIN(TestTaskTreeModel)

#include "test_task_tree_model.moc"
