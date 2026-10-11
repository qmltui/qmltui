#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <Qamui/Core/engine/Animation.h>
#include "../TestPaint.h"
#include <memory>

using namespace qamui;

class TestBehavior : public QObject {
    Q_OBJECT
private slots:
    void boundProperty_data() {
        QTest::addColumn<QString>("declaration");
        QTest::addColumn<QString>("property");
        QTest::newRow("qml-int") << "property int animated: root.requested; x: animated" << "animated";
        QTest::newRow("qml-real") << "property real animated: root.requested; x: animated" << "animated";
        QTest::newRow("native-x") << "x: root.requested" << "x";
    }

    void boundProperty() {
        QFETCH(QString, declaration);
        QFETCH(QString, property);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QString(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import Qamui
            Item {
                id: root
                width: 30; height: 1
                property int requested: 0
                Text {
                    text: "X"
                    %1
                    Behavior on %2 { NumberAnimation { duration: 100 } }
                }
            }
        )").arg(declaration, property).toUtf8(), QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<Item *>(object.get());
        QVERIFY(root);
        auto *animation = root->findChild<PropertyAnimation *>();
        QVERIFY(animation);
        TextGridBuffer buffer;
        QVERIFY(buffer.resize(30, 1));
        auto markerAt = [&](int x) {
            buffer.clear();
            testing::paintItem(root, buffer, 0, 0);
            return QString::fromUtf8(buffer.cellAt(x, 0).character());
        };
        QCOMPARE(markerAt(0), QString("X"));
        for (int target : {10, 20}) {
            QVERIFY(root->setProperty("requested", target));
            QVERIFY(animation->running());
            animation->tick(0);
            animation->tick(50);
            QCOMPARE(markerAt(target - 5), QString("X"));
            animation->tick(50);
            QVERIFY(!animation->running());
            QCOMPARE(markerAt(target), QString("X"));
        }
        // Retarget while still running: restart from the visible value, and
        // give the new target a complete animation duration.
        QVERIFY(root->setProperty("requested", 0));
        animation->tick(0);
        animation->tick(50);
        QCOMPARE(markerAt(10), QString("X"));
        QVERIFY(root->setProperty("requested", 20));
        animation->tick(0);
        QCOMPARE(markerAt(10), QString("X"));
        animation->tick(50);
        QCOMPARE(markerAt(15), QString("X"));
        animation->tick(50);
        QCOMPARE(markerAt(20), QString("X"));
    }

    void boundColor_data() {
        QTest::addColumn<QString>("declaration");
        QTest::addColumn<QString>("property");
        QTest::newRow("qml-string") << "property string animated: root.requested; color: animated" << "animated";
        QTest::newRow("native-color") << "color: root.requested" << "color";
    }

    void boundColor() {
        QFETCH(QString, declaration);
        QFETCH(QString, property);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QString(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import Qamui
            Item {
                id: root
                width: 1; height: 1
                property string requested: "%3"
                Rectangle {
                    width: 1; height: 1
                    %1
                    Behavior on %2 { ColorAnimation { duration: 100 } }
                }
            }
        )").arg(declaration, property, QStringLiteral("#000000")).toUtf8(), QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<Item *>(object.get());
        QVERIFY(root);
        auto *animation = root->findChild<PropertyAnimation *>();
        QVERIFY(animation);
        animation->tick(0);
        animation->tick(100);
        TextGridBuffer buffer;
        QVERIFY(buffer.resize(1, 1));
        auto background = [&]() {
            buffer.clear();
            testing::paintItem(root, buffer, 0, 0);
            return buffer.cellAt(0, 0).bgColor;
        };
        QCOMPARE(background(), 0xff000000u);
        for (const QString &target : {"#ffffff", "#000000"}) {
            QVERIFY(root->setProperty("requested", target));
            QVERIFY(animation->running());
            animation->tick(0);
            animation->tick(50);
            QCOMPARE(background(), 0xff7f7f7fu);
            animation->tick(50);
            QCOMPARE(background(), target == "#ffffff" ? 0xffffffffu : 0xff000000u);
        }
    }

    void withoutAnimationPreservesBinding() {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import Qamui
            Text {
                id: root
                property int requested: 0
                property int animated: root.requested
                text: String(animated)
                Behavior on animated {}
            }
        )", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<Item *>(object.get());
        QVERIFY(root);
        TextGridBuffer buffer;
        QVERIFY(buffer.resize(1, 1));
        for (int target : {1, 2}) {
            QVERIFY(root->setProperty("requested", target));
            buffer.clear();
            testing::paintItem(root, buffer, 0, 0);
            QCOMPARE(QString::fromUtf8(buffer.cellAt(0, 0).character()), QString::number(target));
        }
    }
};

QTEST_GUILESS_MAIN(TestBehavior)
#include "test_behavior.moc"
