#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QmlTui/Core/Items/Repeater.h>
#include "../TestPaint.h"
#include <memory>

using namespace qmltui;

class TestRepeater : public QObject {
    Q_OBJECT
private slots:
    void sourceOrder_data() {
        QTest::addColumn<QString>("layout");
        QTest::addColumn<QString>("model");
        for (const QString& layout : {"Row", "Column"})
            for (const QString& model : {"count", "values", "rows"})
                QTest::newRow(qPrintable(layout + "-" + model)) << layout << model;
    }

    void sourceOrder() {
        QFETCH(QString, layout);
        QFETCH(QString, model);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QString delegate =
            model == "rows" ? "required property string letter; text: letter"
                            : "required property int index; text: String.fromCharCode(66 + index)";
        component.setData(QString(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QtQml.Models
            import QmlTui
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
        )")
                              .arg(layout, model == "rows" ? model : "root." + model, delegate)
                              .toUtf8(),
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto* root = qobject_cast<Item*>(object.get());
        QVERIFY(root);
        TextGridBuffer buffer;
        QVERIFY(buffer.resize(20, 20));
        auto rendered = [&]() {
            buffer.clear();
            testing::paintItem(root, buffer, 0, 0);
            QString result;
            for (int i = 0; i < 20; ++i)
                result += QString::fromUtf8(
                    buffer.cellAt(layout == "Row" ? i : 0, layout == "Row" ? 0 : i).character());
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

    void delegateChooserAndIntegerModelData() {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QtQml.Models
            import Qt.labs.qmlmodels
            import QmlTui
            Column {
                id: root
                width: 20; height: 10
                ListModel {
                    id: typedRows
                    ListElement { kind: "header"; label: "HDR" }
                    ListElement { kind: "item"; label: "ITM" }
                }
                Repeater {
                    model: typedRows
                    delegate: DelegateChooser {
                        role: "kind"
                        DelegateChoice {
                            roleValue: "header"
                            delegate: Text {
                                required property string label
                                text: "[" + label + "]"
                            }
                        }
                        DelegateChoice {
                            roleValue: "item"
                            delegate: Text {
                                required property string label
                                text: "- " + label
                            }
                        }
                    }
                }
                Repeater {
                    model: [{ kind: "a", label: "S1" }, { kind: "b", label: "S2" }]
                    delegate: DelegateChooser {
                        role: "kind"
                        DelegateChoice {
                            roleValue: "a"
                            delegate: Text { required property string label; text: "A:" + label }
                        }
                        DelegateChoice {
                            roleValue: "b"
                            delegate: Text { required property string label; text: "B:" + label }
                        }
                    }
                }
                Repeater {
                    model: 2
                    delegate: Text {
                        required property int modelData
                        text: "#" + modelData
                    }
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        std::unique_ptr<QObject> object(component.create());
        auto* root = qobject_cast<Item*>(object.get());
        QVERIFY(root);

        TextGridBuffer buffer;
        QVERIFY(buffer.resize(20, 10));
        testing::paintItem(root, buffer, 0, 0);
        auto line = [&](int y) {
            QString s;
            for (int x = 0; x < 20; ++x)
                s += QString::fromUtf8(buffer.cellAt(x, y).character());
            return s.trimmed();
        };
        QCOMPARE(line(0), QStringLiteral("[HDR]"));
        QCOMPARE(line(1), QStringLiteral("- ITM"));
        QCOMPARE(line(2), QStringLiteral("A:S1"));
        QCOMPARE(line(3), QStringLiteral("B:S2"));
        QCOMPARE(line(4), QStringLiteral("#0"));
        QCOMPARE(line(5), QStringLiteral("#1"));
    }
};

QTEST_GUILESS_MAIN(TestRepeater)
#include "test_repeater.moc"
