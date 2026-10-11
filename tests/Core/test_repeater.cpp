#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <Qamui/Core/items/Repeater.h>
#include "../TestPaint.h"
#include <memory>

using namespace qamui;

class TestRepeater : public QObject {
    Q_OBJECT
private slots:
    void sourceOrder_data() {
        QTest::addColumn<QString>("layout");
        QTest::addColumn<QString>("model");
        for (const QString &layout : {"Row", "Column"})
            for (const QString &model : {"count", "values", "rows"})
                QTest::newRow(qPrintable(layout + "-" + model)) << layout << model;
    }

    void sourceOrder() {
        QFETCH(QString, layout);
        QFETCH(QString, model);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QString delegate = model == "rows"
            ? "required property string letter; text: letter"
            : "required property int index; text: String.fromCharCode(66 + index)";
        component.setData(QString(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QtQml.Models
            import Qamui
            %1 {
                id: root
                property int count: 2
                property var values: ["B", "C"]
                ListModel {
                    id: rows
                    ListElement { letter: "B" }
                    ListElement { letter: "C" }
                }
                Text { text: "A" }
                Repeater { model: %2; delegate: Text { %3 } }
                Text { text: "Z" }
                function insertRow() { rows.insert(1, {letter: "X"}) }
                function moveRow() { rows.move(2, 0, 1) }
                function removeRow() { rows.remove(1) }
                function resetRows() { rows.clear(); rows.append({letter: "R"}) }
            }
        )").arg(layout, model == "rows" ? model : "root." + model, delegate).toUtf8(), QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto *root = qobject_cast<Item *>(object.get());
        QVERIFY(root);
        TextGridBuffer buffer;
        QVERIFY(buffer.resize(20, 20));
        auto rendered = [&]() {
            buffer.clear();
            testing::paintItem(root, buffer, 0, 0);
            QString result;
            for (int i = 0; i < 20; ++i)
                result += QString::fromUtf8(buffer.cellAt(layout == "Row" ? i : 0,
                                                         layout == "Row" ? 0 : i).character());
            return result.trimmed();
        };
        QCOMPARE(rendered(), QString("ABCZ"));
        if (model == "rows") {
            QVERIFY(QMetaObject::invokeMethod(root, "insertRow"));
            QCOMPARE(rendered(), QString("ABXCZ"));
            QVERIFY(QMetaObject::invokeMethod(root, "moveRow"));
            QCOMPARE(rendered(), QString("ACBXZ"));
            QVERIFY(QMetaObject::invokeMethod(root, "removeRow"));
            QCOMPARE(rendered(), QString("ACXZ"));
            QVERIFY(QMetaObject::invokeMethod(root, "resetRows"));
            QCOMPARE(rendered(), QString("ARZ"));
        } else {
            QVERIFY(root->setProperty("count", 3));
            QVERIFY(root->setProperty("values", QVariantList{"B", "C", "D"}));
            QCOMPARE(rendered(), QString("ABCDZ"));
            QVERIFY(root->setProperty("count", 0));
            QVERIFY(root->setProperty("values", QVariantList{}));
            QCOMPARE(rendered(), QString("AZ"));
        }
    }
};

QTEST_GUILESS_MAIN(TestRepeater)
#include "test_repeater.moc"
