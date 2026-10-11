#include "WifiManager.h"

#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

class TestWifiManager : public QObject
{
    Q_OBJECT

private slots:
    void mockBackendIsDeterministic();
    void observeBackendRejectsMutations();
    void statusCommandDoesNotBlockTimers();
};

void TestWifiManager::mockBackendIsDeterministic()
{
    qputenv("QAMUI_STARTER_BACKEND", "mock");
    WifiManager manager;

    QCOMPARE(manager.networks().size(), 2);
    QCOMPARE(manager.networks().at(0).toMap().value("ssid").toString(), QStringLiteral("Qamui Lab"));
    QVERIFY(!manager.scanning());
    QVERIFY(!manager.connected());
}

void TestWifiManager::observeBackendRejectsMutations()
{
    qputenv("QAMUI_STARTER_BACKEND", "observe");
    WifiManager manager;
    QSignalSpy connectionResult(&manager, &WifiManager::connectionResult);

    manager.scan();
    QVERIFY(manager.lastError().contains(QStringLiteral("disabled")));
    QVERIFY(!manager.scanning());

    manager.connectToNetwork(QStringLiteral("Do not join"), QStringLiteral("secret"));
    QCOMPARE(connectionResult.size(), 1);
    QCOMPARE(connectionResult.at(0).at(0).toBool(), false);
    QVERIFY(!manager.connected());
}

void TestWifiManager::statusCommandDoesNotBlockTimers()
{
    QTemporaryDir tools;
    QVERIFY(tools.isValid());
    const QString invocationLog = tools.filePath(QStringLiteral("invocations"));
    QFile fakeWpaCli(tools.filePath(QStringLiteral("wpa_cli")));
    QVERIFY(fakeWpaCli.open(QIODevice::WriteOnly | QIODevice::Text));
    fakeWpaCli.write(QString("#!/bin/sh\nprintf x >> '%1'\n/bin/sleep 1\nexit 1\n")
                         .arg(invocationLog).toUtf8());
    fakeWpaCli.close();
    QVERIFY(fakeWpaCli.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                     QFileDevice::ExeOwner));

    const QByteArray previousPath = qgetenv("PATH");
    qputenv("PATH", tools.path().toUtf8());
    qputenv("QAMUI_STARTER_BACKEND", "observe");

    bool timerFired = false;
    QElapsedTimer elapsed;
    elapsed.start();
    {
        WifiManager manager;
        manager.refreshStatus();
        manager.refreshStatus(); // overlapping status work must be ignored
        QTimer::singleShot(20, [&timerFired] { timerFired = true; });
        QTRY_VERIFY_WITH_TIMEOUT(timerFired, 150);
        QVERIFY2(elapsed.elapsed() < 200, "a status command blocked the Qt event loop");
        QTest::qWait(850); // allow the command timeout to finish before teardown
        QFile calls(invocationLog);
        QVERIFY(calls.open(QIODevice::ReadOnly));
        QCOMPARE(calls.readAll(), QByteArray("x"));
    }

    qputenv("PATH", previousPath);
}

QTEST_GUILESS_MAIN(TestWifiManager)

#include "test_wifi_manager.moc"
