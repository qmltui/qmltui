// Plan 07 WP4: Qt's standard item roles (foreground, background, alignment,
// check state, decoration glyph) and item flags reach the terminal, both
// through the built-in painters of TableView and TreeView and through the
// StandardItemDelegate control.
#include <QTest>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QAbstractListModel>
#include <QColor>
#include <QmlTui/Core/Items/ListView.h>
#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/TreeView.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Primitives/Color.h>
#include "../TestPaint.h"
#include <memory>

using namespace qmltui;

namespace {

bool hasAttribute(const Cell& cell, TextAttribute attribute) {
    return (static_cast<uint8_t>(cell.attributes) & static_cast<uint8_t>(attribute)) != 0;
}

// alpha: red, right aligned. beta: checkable and checked, blue background.
// gamma: disabled. delta: "*" decoration.
std::unique_ptr<QStandardItemModel> styledModel() {
    auto model = std::make_unique<QStandardItemModel>();
    auto* alpha = new QStandardItem(QStringLiteral("alpha"));
    alpha->setForeground(QColor(Qt::red));
    alpha->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto* beta = new QStandardItem(QStringLiteral("beta"));
    beta->setCheckable(true);
    beta->setCheckState(Qt::Checked);
    beta->setBackground(QColor(Qt::blue));
    auto* gamma = new QStandardItem(QStringLiteral("gamma"));
    gamma->setEnabled(false);
    auto* delta = new QStandardItem(QStringLiteral("delta"));
    delta->setData(QStringLiteral("*"), Qt::DecorationRole);
    for (QStandardItem* item : {alpha, beta, gamma, delta})
        model->appendRow(item);
    return model;
}

// Colors and alignment as plain values, the way a Core-only model would
// provide them without QtGui types.
class PlainStyledModel final : public QAbstractListModel {
public:
    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : 2; }
    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid()) return {};
        switch (role) {
        case Qt::DisplayRole:
            return QStringLiteral("row %1").arg(index.row());
        case Qt::ForegroundRole:
            return index.row() == 0 ? QStringLiteral("green") : QVariant();
        case Qt::TextAlignmentRole:
            return index.row() == 0 ? int(Qt::AlignHCenter) : QVariant();
        case Qt::CheckStateRole:
            return index.row() == 0 ? int(Qt::Checked) : QVariant();
        default:
            return {};
        }
    }
    Qt::ItemFlags flags(const QModelIndex& index) const override {
        Qt::ItemFlags result = QAbstractListModel::flags(index);
        if (index.row() == 0) result |= Qt::ItemIsUserCheckable;
        if (index.row() == 1) result &= ~Qt::ItemIsEnabled;
        return result;
    }
};

struct Fixture {
    QQmlEngine engine;
    QQmlComponent component{&engine};
    std::unique_ptr<QObject> object;
    Item* root = nullptr;
    TextGridBuffer buffer;
    int width = 20;

    bool create(QObject* model, const QString& qml, int w = 20, int h = 8) {
        width = w;
        engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../qml"));
        engine.rootContext()->setContextProperty("testModel", model);
        component.setData(qml.toUtf8(), QUrl());
        if (!component.isReady()) return false;
        object.reset(component.create());
        root = qobject_cast<Item*>(object.get());
        if (!root || !buffer.resize(w, h)) return false;
        paint();
        return true;
    }
    void paint() {
        buffer.clear();
        testing::paintItem(root, buffer, 0, 0);
    }
    QString line(int y) const {
        QString result;
        for (int x = 0; x < width; ++x)
            result += QString::fromUtf8(buffer.cellAt(x, y).character());
        return result;
    }
};

const uint32_t red = Color::resolve(QStringLiteral("#ff0000"), 1);
const uint32_t blue = Color::resolve(QStringLiteral("#0000ff"), 1);
const uint32_t green = Color::resolve(QStringLiteral("green"), 1);

} // namespace

class TestStandardRoles : public QObject {
    Q_OBJECT
private slots:
    void tableViewBuiltInPainter() {
        auto model = styledModel();
        Fixture f;
        QVERIFY2(f.create(model.get(), QStringLiteral(R"(
            import QmlTui
            TableView { width: 20; height: 8; model: testModel; columns: [{title: "Name", width: 20}] }
        )")),
                 qPrintable(f.component.errorString()));
        // Header occupies two rows.
        QCOMPARE(f.line(2), QStringLiteral("               alpha"));
        QCOMPARE(f.buffer.cellAt(19, 2).fgColor, red);
        QCOMPARE(f.line(3).trimmed(), QStringLiteral("[x] beta"));
        QCOMPARE(f.buffer.cellAt(0, 3).bgColor, blue);
        QCOMPARE(f.line(4).trimmed(), QStringLiteral("gamma"));
        QVERIFY(hasAttribute(f.buffer.cellAt(0, 4), TextAttribute::Dim));
        QVERIFY(!hasAttribute(f.buffer.cellAt(0, 3), TextAttribute::Dim));
        QCOMPARE(f.line(5).trimmed(), QStringLiteral("* delta"));

        // Live: unchecking repaints the box; the selected row keeps selection colors.
        model->item(1)->setCheckState(Qt::Unchecked);
        f.paint();
        QCOMPARE(f.line(3).trimmed(), QStringLiteral("[ ] beta"));
        auto* table = qobject_cast<TableView*>(f.root);
        QVERIFY(table);
        table->setSelectedRow(0);
        f.paint();
        QVERIFY(f.buffer.cellAt(19, 2).fgColor != red);
    }

    void treeViewBuiltInPainter() {
        auto model = styledModel();
        Fixture f;
        QVERIFY2(f.create(model.get(), QStringLiteral(R"(
            import QmlTui
            TreeView { width: 20; height: 8; model: testModel; currentIndex: -1 }
        )")),
                 qPrintable(f.component.errorString()));
        // The first node is current by default and drawn with selection colors.
        auto* tree = qobject_cast<TreeView*>(f.root);
        QVERIFY(tree);
        tree->setCurrentIndex(3);
        f.paint();
        QCOMPARE(f.line(0).trimmed(), QStringLiteral("alpha"));
        QCOMPARE(f.buffer.cellAt(4, 0).fgColor, red);
        QCOMPARE(f.line(1).trimmed(), QStringLiteral("[x] beta"));
        QCOMPARE(f.buffer.cellAt(4, 1).bgColor, blue);
        QVERIFY(hasAttribute(f.buffer.cellAt(4, 2), TextAttribute::Dim));
        QCOMPARE(f.line(3).trimmed(), QStringLiteral("* delta"));
    }

    void standardRolesInListView() {
        auto model = styledModel();
        Fixture f;
        QVERIFY2(f.create(model.get(), QStringLiteral(R"(
            import QtQml
            import QmlTui
            ListView {
                width: 20; height: 8; model: testModel
                delegate: Item {
                    id: d
                    required property var model
                    property bool current: ListView.isCurrentItem
                    readonly property string display: model && model.display ? String(model.display) : ""
                    readonly property bool checkable: !!model && model.checkable === true
                    readonly property int checkState: !!model && model.checkState !== undefined ? Number(model.checkState) : -1
                    readonly property bool itemEnabled: !(!!model && model.enabled === false)
                    readonly property string foreground: !!model && typeof model.foreground === "string" ? model.foreground : ""
                    readonly property string background: !!model && typeof model.background === "string" ? model.background : ""
                    readonly property string decoration: !!model && typeof model.decoration === "string" ? model.decoration : ""
                    readonly property string alignment: !!model && typeof model.textAlignment === "string" ? model.textAlignment : "left"
                    readonly property string prefix: (decoration !== "" ? decoration + " " : "")
                        + (checkState < 0 ? "" : checkState === Qt.Checked ? "[x] " : "[ ] ")
                    function toggle() {
                        if (!checkable || !itemEnabled || !model) return false
                        const index = model.modelIndex
                        if (!index || !index.valid || !index.model) return false
                        const next = checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
                        return index.model.setData(index, next, Qt.CheckStateRole)
                    }
                    width: parent ? parent.width : 20
                    height: 1
                    enabled: itemEnabled
                    Rectangle {
                        anchors.fill: parent
                        visible: d.background !== "" && !d.current
                        color: d.background !== "" ? d.background : "default"
                    }
                    Text {
                        anchors.fill: parent
                        text: d.prefix + d.display
                        color: d.foreground !== "" ? d.foreground : "default"
                        dim: !d.itemEnabled
                        bold: d.current
                        reversed: d.current
                        horizontalAlignment: d.alignment === "right" ? Qt.AlignRight
                                           : d.alignment === "center" ? Qt.AlignHCenter : Qt.AlignLeft
                    }
                }
            }
        )")),
                 qPrintable(f.component.errorString()));
        QCOMPARE(f.line(0), QStringLiteral("               alpha"));
        QCOMPARE(f.buffer.cellAt(19, 0).fgColor, red);
        QCOMPARE(f.line(1).trimmed(), QStringLiteral("[x] beta"));
        QCOMPARE(f.buffer.cellAt(0, 1).bgColor, blue);
        QCOMPARE(f.line(2).trimmed(), QStringLiteral("gamma"));
        QVERIFY(hasAttribute(f.buffer.cellAt(0, 2), TextAttribute::Dim));
        QCOMPARE(f.line(3).trimmed(), QStringLiteral("* delta"));

        // toggle() writes Qt::CheckStateRole back through setData().
        Item* betaDelegate = nullptr;
        Item* gammaDelegate = nullptr;
        for (Item* item : f.root->findChildren<Item*>()) {
            if (item->metaObject()->indexOfMethod("toggle()") < 0) continue;
            if (item->property("display").toString() == "beta") betaDelegate = item;
            if (item->property("display").toString() == "gamma") gammaDelegate = item;
        }
        QVERIFY(betaDelegate);
        QVERIFY(gammaDelegate);
        QVariant toggled;
        QVERIFY(QMetaObject::invokeMethod(betaDelegate, "toggle", Q_RETURN_ARG(QVariant, toggled)));
        QVERIFY(toggled.toBool());
        QCOMPARE(model->item(1)->checkState(), Qt::Unchecked);
        f.paint();
        QCOMPARE(f.line(1).trimmed(), QStringLiteral("[ ] beta"));
        // Not checkable: toggle() refuses and leaves the model alone.
        QVERIFY(
            QMetaObject::invokeMethod(gammaDelegate, "toggle", Q_RETURN_ARG(QVariant, toggled)));
        QVERIFY(!toggled.toBool());
        QVERIFY(!gammaDelegate->enabled());

        // Live style change through dataChanged.
        model->item(3)->setForeground(QColor(Qt::red));
        f.paint();
        QCOMPARE(f.buffer.cellAt(0, 3).fgColor, red);
    }

    void plainValuesWithoutGuiTypes() {
        PlainStyledModel model;
        Fixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            import QmlTui
            TableView { width: 20; height: 8; model: testModel; columns: [{title: "Name", width: 20}] }
        )")),
                 qPrintable(f.component.errorString()));
        // "[x] row 0" centered in 20 cells.
        QCOMPARE(f.line(2), QStringLiteral("     [x] row 0      "));
        QCOMPARE(f.buffer.cellAt(5, 2).fgColor, green);
        QCOMPARE(f.line(3).trimmed(), QStringLiteral("row 1"));
        QVERIFY(hasAttribute(f.buffer.cellAt(0, 3), TextAttribute::Dim));
    }

    void modelRoleNamesWin() {
        // A model that names its own "foreground" role keeps its value.
        class NamedModel final : public QAbstractListModel {
        public:
            int rowCount(const QModelIndex& parent = {}) const override {
                return parent.isValid() ? 0 : 1;
            }
            QVariant data(const QModelIndex& index, int role) const override {
                if (!index.isValid()) return {};
                if (role == Qt::DisplayRole) return QStringLiteral("named");
                if (role == Qt::UserRole + 1) return QStringLiteral("custom-value");
                if (role == Qt::ForegroundRole) return QStringLiteral("red");
                return {};
            }
            QHash<int, QByteArray> roleNames() const override {
                return {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "foreground"}};
            }
        } model;
        Fixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            import QmlTui
            ListView {
                width: 20; height: 8; model: testModel
                delegate: Text { required property string foreground; text: foreground }
            }
        )")),
                 qPrintable(f.component.errorString()));
        QCOMPARE(f.line(0).trimmed(), QStringLiteral("custom-value"));
    }

    void boundDelegatesUnsetNumericRoles() {
        // Under pragma ComponentBehavior: Bound, delegates declaring required properties
        // for numeric item roles (enabled, checkable, checkState, foreground) must succeed
        // even when rows leave those roles unset in ListView, GridView, and Repeater.
        QStringListModel model({QStringLiteral("one"), QStringLiteral("two")});
        Fixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            pragma ComponentBehavior: Bound
            import QmlTui
            Column {
                width: 20
                height: 8
                ListView {
                    width: 20
                    height: 2
                    model: testModel
                    delegate: Text {
                        required property string display
                        required property bool enabled
                        required property bool checkable
                        width: 20
                        height: 1
                        text: (enabled && !checkable ? "+" : "-") + display
                    }
                }
                GridView {
                    width: 20
                    height: 1
                    cellWidth: 10
                    cellHeight: 1
                    model: testModel
                    delegate: Text {
                        required property string display
                        required property bool enabled
                        text: (enabled ? "G:" : "X:") + display
                    }
                }
                Row {
                    Repeater {
                        model: testModel
                        Text {
                            required property string display
                            required property bool enabled
                            width: 10
                            height: 1
                            text: (enabled ? "R:" : "X:") + display
                        }
                    }
                }
            }
        )")),
                 qPrintable(f.component.errorString()));
        QCOMPARE(f.line(0).trimmed(), QStringLiteral("+one"));
        QCOMPARE(f.line(1).trimmed(), QStringLiteral("+two"));
        QCOMPARE(f.line(2).simplified(), QStringLiteral("G:one G:two"));
        QCOMPARE(f.line(3).simplified(), QStringLiteral("R:one R:two"));
    }
};

QTEST_MAIN(TestStandardRoles)
#include "test_standard_roles.moc"
