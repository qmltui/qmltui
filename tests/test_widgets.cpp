#include <QtTest/QtTest>
#include <Sparkline.h>
#include <BigText.h>
#include <TableView.h>
#include <TreeView.h>
#include <ToastManager.h>
#include <TabHeader.h>
#include <Gauge.h>
#include <Paragraph.h>
#include <TextGridBuffer.h>
#include <MouseEvent.h>

using namespace qtui;

class TestWidgets : public QObject {
    Q_OBJECT

private slots:
    void testSparkline() {
        Sparkline sparkline;
        sparkline.setWidth(10);
        sparkline.setHeight(1);
        sparkline.setValues(QVariantList{10.0, 20.0, 30.0, 40.0, 50.0});

        TextGridBuffer buffer;
        buffer.resize(10, 1);
        sparkline.render(buffer, 0, 0);

        QVERIFY(buffer.cellAt(4, 0).character[0] != ' ');
    }

    void testBigText() {
        BigText bigText;
        bigText.setText("123");

        TextGridBuffer buffer;
        buffer.resize(20, 3);
        bigText.render(buffer, 0, 0);

        QCOMPARE(buffer.cellAt(0, 0).character[0], ' ');
        QVERIFY(buffer.cellAt(1, 0).character[0] != '\0');
    }

    void testTableView() {
        TableView table;
        table.setWidth(40);
        table.setHeight(6);

        table.setColumns(QVariantList{
            QVariantMap{{"title", "ID"}, {"width", 10}},
            QVariantMap{{"title", "NAME"}, {"width", 20}}
        });

        table.setModel(QVariantList{
            QVariantList{"1", "Alice"},
            QVariantList{"2", "Bob"}
        });

        TextGridBuffer buffer;
        buffer.resize(40, 6);
        table.render(buffer, 0, 0);

        QCOMPARE(table.selectedRow(), 0);
        table.setSelectedRow(1);
        QCOMPARE(table.selectedRow(), 1);
    }

    void testTreeView() {
        TreeView tree;
        tree.setWidth(30);
        tree.setHeight(5);

        tree.setRootNode(QVariantMap{
            {"text", "root"},
            {"expanded", true},
            {"children", QVariantList{
                QVariantMap{{"text", "child1"}}
            }}
        });

        TextGridBuffer buffer;
        buffer.resize(30, 5);
        tree.render(buffer, 0, 0);

        QCOMPARE(tree.selectedIndex(), 0);

        MouseEvent mouseEv(0, 0, 0);
        tree.mousePressEvent(&mouseEv);
        QVERIFY(mouseEv.isAccepted());
        QCOMPARE(tree.selectedIndex(), 0);
    }

    void testToastManager() {
        ToastManager toast;
        toast.setWidth(30);
        toast.setHeight(5);

        toast.showToast("Test", "Message", "info", 1000);
        TextGridBuffer buffer;
        buffer.resize(30, 5);
        toast.render(buffer, 0, 0);
    }

    void testTabHeader() {
        TabHeader header;
        header.setWidth(40);
        header.setHeight(1);
        header.setTabs(QVariantList{"Overview", "Metrics", "Settings"});

        QCOMPARE(header.currentIndex(), 0);
        header.setCurrentIndex(1);
        QCOMPARE(header.currentIndex(), 1);

        TextGridBuffer buffer;
        buffer.resize(40, 1);
        header.render(buffer, 0, 0);
    }

    void testGauge() {
        Gauge gauge;
        gauge.setWidth(30);
        gauge.setHeight(1);
        gauge.setLabel("CPU: ");
        gauge.setValue(75.0);

        TextGridBuffer buffer;
        buffer.resize(30, 1);
        gauge.render(buffer, 0, 0);

        QCOMPARE(gauge.value(), 75.0);
    }

    void testParagraph() {
        Paragraph p;
        p.setWidth(20);
        p.setHeight(5);
        p.setText("Qtui is a modern reactive QML Terminal UI framework.");

        TextGridBuffer buffer;
        buffer.resize(20, 5);
        p.render(buffer, 0, 0);
    }
};

QTEST_MAIN(TestWidgets)
#include "test_widgets.moc"
