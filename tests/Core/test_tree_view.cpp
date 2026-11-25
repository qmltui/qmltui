// Plan 07 WP6: TreeView over QAbstractItemModel keeps expansion and selection
// stable across renames, inserts, sorting, and proxies, and applies
// dataChanged in place without re-creating delegates.
#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QmlTui/Core/Items/TreeView.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <memory>

using namespace qmltui;

namespace {

std::unique_ptr<QStandardItemModel> makeTree(int groups, int leaves) {
    auto model = std::make_unique<QStandardItemModel>();
    for (int g = 0; g < groups; ++g) {
        auto* group = new QStandardItem(QStringLiteral("group %1").arg(g));
        for (int l = 0; l < leaves; ++l)
            group->appendRow(new QStandardItem(QStringLiteral("leaf %1.%2").arg(g).arg(l)));
        model->appendRow(group);
    }
    return model;
}

const QString delegateQml = QStringLiteral(R"(
    import QmlTui
    TreeView {
        width: 40; height: 8
        model: testModel
        delegate: Text {
            required property int depth
            required property bool expanded
            required property bool hasChildren
            required property bool current
            required property string display
            text: (current ? ">" : " ") + " ".repeat(depth * 2) + (hasChildren ? (expanded ? "- " : "+ ") : "  ") + display
        }
    }
)");
const QString builtinQml = QStringLiteral(R"(
    import QmlTui
    TreeView { width: 40; height: 8; model: testModel }
)");

struct TreeFixture {
    QQmlEngine engine;
    QQmlComponent component{&engine};
    std::unique_ptr<QObject> object;
    TreeView* tree = nullptr;
    TextGridBuffer buffer;

    bool create(QObject* model, const QString& qml) {
        engine.rootContext()->setContextProperty("testModel", model);
        component.setData(qml.toUtf8(), QUrl());
        if (!component.isReady()) return false;
        object.reset(component.create());
        tree = qobject_cast<TreeView*>(object.get());
        if (!tree || !buffer.resize(40, 8)) return false;
        paint();
        return true;
    }
    void paint() {
        buffer.clear();
        testing::paintItem(tree, buffer, 0, 0);
    }
    QString line(int y) const {
        QString result;
        for (int x = 0; x < 40; ++x)
            result += QString::fromUtf8(buffer.cellAt(x, y).character());
        return result.trimmed();
    }
    QString text() const {
        QStringList lines;
        for (int y = 0; y < 8; ++y)
            lines << line(y);
        return lines.join('\n');
    }
    QSet<Item*> items() const {
        const auto list = tree->findChildren<Item*>();
        return QSet<Item*>(list.cbegin(), list.cend());
    }
};

} // namespace

class TestTreeView : public QObject {
    Q_OBJECT
private slots:
    void renameKeepsExpansionAndDelegates() {
        auto model = makeTree(2, 2);
        TreeFixture f;
        QVERIFY2(f.create(model.get(), delegateQml), qPrintable(f.component.errorString()));
        f.tree->expand(1);
        f.paint();
        QVERIFY2(f.text().contains("leaf 1.0"), qPrintable(f.text()));
        const QSet<Item*> before = f.items();

        // Renaming the expanded branch used to change its string key and
        // collapse it.
        model->item(1)->setText(QStringLiteral("renamed group"));
        f.paint();
        QVERIFY2(f.text().contains("renamed group"), qPrintable(f.text()));
        QVERIFY(f.text().contains("leaf 1.1"));
        QVERIFY(f.tree->isExpanded(1));
        QCOMPARE(f.items(), before);

        // Renaming a leaf is applied in place.
        model->item(1)->child(0)->setText(QStringLiteral("first leaf"));
        f.paint();
        QVERIFY(f.text().contains("first leaf"));
        QVERIFY(!f.text().contains("leaf 1.0"));
        QCOMPARE(f.items(), before);
    }

    void insertAboveKeepsExpansionAndSelection() {
        auto model = makeTree(3, 2);
        TreeFixture f;
        QVERIFY2(f.create(model.get(), builtinQml), qPrintable(f.component.errorString()));
        f.tree->expand(2);
        f.tree->setCurrentIndex(3); // leaf 2.0
        f.paint();
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("leaf 2.0"));

        auto* inserted = new QStandardItem(QStringLiteral("group new"));
        inserted->appendRow(new QStandardItem(QStringLiteral("leaf new")));
        model->insertRow(0, inserted);
        f.paint();
        QCOMPARE(f.line(0), QStringLiteral("[+] group new"));
        QVERIFY(f.tree->isExpanded(3));
        QCOMPARE(f.tree->currentIndex(), 4);
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("leaf 2.0"));

        // Removing rows under a collapsed, visible node is observed (its
        // expander disappears); removing under a hidden node is not.
        model->item(1)->removeRows(0, 2);
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("group 0"));
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("leaf 2.0"));
    }

    void dataChangedDoesNotScrollOrRecreate() {
        QStandardItemModel model;
        for (int i = 0; i < 40; ++i)
            model.appendRow(new QStandardItem(QStringLiteral("node %1").arg(i)));
        TreeFixture f;
        QVERIFY2(f.create(&model, delegateQml), qPrintable(f.component.errorString()));
        f.tree->setCurrentIndex(30);
        f.paint();
        const QString firstLine = f.line(0);
        QVERIFY(firstLine.contains("node 23"));
        const QSet<Item*> before = f.items();
        QSignalSpy currentData(f.tree, &TreeView::currentItemDataChanged);

        model.item(25)->setText(QStringLiteral("changed 25"));
        model.item(3)->setText(QStringLiteral("changed 3")); // off-screen
        f.paint();
        QCOMPARE(f.line(0), firstLine);
        QVERIFY2(f.text().contains("changed 25"), qPrintable(f.text()));
        QCOMPARE(f.items(), before);
        QCOMPARE(f.tree->currentIndex(), 30);
        QCOMPARE(currentData.size(), 0);

        model.item(30)->setText(QStringLiteral("changed 30"));
        f.paint();
        QCOMPARE(currentData.size(), 1);
        QCOMPARE(f.tree->currentItemData().value("display").toString(),
                 QStringLiteral("changed 30"));
    }

    void leafGainingChildrenBecomesBranch() {
        auto model = makeTree(2, 1);
        TreeFixture f;
        QVERIFY2(f.create(model.get(), builtinQml), qPrintable(f.component.errorString()));
        f.tree->expand(0);
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("leaf 0.0"));
        model->item(0)->child(0)->appendRow(new QStandardItem(QStringLiteral("deep")));
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("[+] leaf 0.0"));
        f.tree->expand(1);
        f.paint();
        QCOMPARE(f.line(2), QStringLiteral("deep"));
    }

    void expandToIndexOpensAncestors() {
        auto model = makeTree(2, 2);
        model->item(1)->child(1)->appendRow(new QStandardItem(QStringLiteral("target")));
        TreeFixture f;
        QVERIFY2(f.create(model.get(), builtinQml), qPrintable(f.component.errorString()));
        const QModelIndex target = model->item(1)->child(1)->child(0)->index();
        f.tree->expandToIndex(target);
        f.paint();
        QVERIFY2(f.text().contains("target"), qPrintable(f.text()));
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("target"));
        QVERIFY(f.tree->isExpanded(1));
        QVERIFY(!f.tree->isExpanded(0));
    }

    void sortingProxyKeepsExpansion() {
        auto model = makeTree(3, 1);
        QSortFilterProxyModel proxy;
        proxy.setSourceModel(model.get());
        TreeFixture f;
        QVERIFY2(f.create(&proxy, builtinQml), qPrintable(f.component.errorString()));
        f.tree->expand(0); // group 0
        f.paint();
        QCOMPARE(f.line(0), QStringLiteral("[-] group 0"));
        QCOMPARE(f.line(1), QStringLiteral("leaf 0.0"));

        proxy.sort(0, Qt::DescendingOrder);
        f.paint();
        QCOMPARE(f.line(0), QStringLiteral("[+] group 2"));
        QCOMPARE(f.line(2), QStringLiteral("[-] group 0"));
        QCOMPARE(f.line(3), QStringLiteral("leaf 0.0"));

        // Non-recursive filter: "1" keeps "group 1" and its "leaf 1.0".
        proxy.setFilterFixedString(QStringLiteral("1"));
        f.paint();
        QCOMPARE(f.line(0), QStringLiteral("[+] group 1"));
        QCOMPARE(f.line(1), QString());
        // A proxy invalidates the persistent indexes of filtered-out rows, so
        // their expansion state is gone when they come back (as in Qt Quick).
        proxy.setFilterFixedString(QString());
        f.paint();
        QCOMPARE(f.line(2), QStringLiteral("[+] group 0"));
        QCOMPARE(f.line(3), QString());
    }

    // A Text root's own text binding is the delegate author's; TreeView must
    // not overwrite it with the node label (it used to).
    void textRootKeepsItsBinding() {
        auto model = makeTree(2, 1);
        TreeFixture f;
        QVERIFY2(f.create(model.get(), QStringLiteral(R"(
            import QmlTui
            TreeView {
                width: 40; height: 8; model: testModel
                delegate: Text { required property string display; required property int depth
                                 text: ">" + " ".repeat(depth) + display; color: "blue" }
            }
        )")),
                 qPrintable(f.component.errorString()));
        QCOMPARE(f.line(0), QStringLiteral(">group 0"));
        f.tree->expand(0);
        f.tree->setCurrentIndex(1);
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("> leaf 0.0"));
        // Scrolling (position-only updates) and renames keep the binding too.
        model->item(0)->child(0)->setText(QStringLiteral("renamed"));
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("> renamed"));
        // Delegates without declared properties still get them dynamically.
        QVERIFY(f.tree->findChildren<Item*>().first()->property("index").isValid());
    }

    void resetCollapsesEverything() {
        auto model = makeTree(2, 1);
        TreeFixture f;
        QVERIFY2(f.create(model.get(), builtinQml), qPrintable(f.component.errorString()));
        f.tree->expandAll();
        f.paint();
        QCOMPARE(f.line(1), QStringLiteral("leaf 0.0"));
        model->clear();
        model->appendRow(new QStandardItem(QStringLiteral("fresh")));
        model->item(0)->appendRow(new QStandardItem(QStringLiteral("child")));
        f.paint();
        QCOMPARE(f.line(0), QStringLiteral("[+] fresh"));
        QCOMPARE(f.line(1), QString());
    }

    void spansAndNodeFormatAndBranchColor() {
        QStandardItemModel model;
        Span s1(QStringLiteral("root"), Color("#ff0000"));
        s1.setBold(true);
        Span s2(QStringLiteral("-tag"), Color("#00ff00"));
        s2.setUnderline(true);

        auto* rootItem = new QStandardItem();
        rootItem->setData(QVariant::fromValue(QList<Span>{s1, s2}), Qt::DisplayRole);
        auto* leafItem = new QStandardItem(QStringLiteral("leaf"));
        rootItem->appendRow(leafItem);
        model.appendRow(rootItem);

        TreeFixture f;
        QVERIFY2(f.create(&model, QStringLiteral(R"(
            import QmlTui
            TreeView {
                width: 40; height: 8
                model: testModel
                textColor: "#cccccc"
                branchColor: "#ff8800"
                nodeFormat: (row, val) => {
                    if (row === 1) {
                        return [
                            QmlTui.span(val + ":", { color: "#00ffff", italic: true }),
                            QmlTui.span("ok", { color: "#00ff88", bold: true })
                        ];
                    }
                    return undefined;
                }
            }
        )")),
                 qPrintable(f.component.errorString()));

        f.tree->expand(0);
        f.tree->setCurrentIndex(1);
        f.paint();

        // Row 0 is unselected: "[-] root-tag"
        QCOMPARE(f.line(0), QStringLiteral("[-] root-tag"));
        // Branch icon "[-] " at x=0..3 has branchColor #ff8800
        QCOMPARE(f.buffer.cellAt(0, 0).fgColor, 0xffff8800u);
        // "root" at x=4..7 has span s1 (#ff0000, Bold)
        QCOMPARE(f.buffer.cellAt(4, 0).fgColor, 0xffff0000u);
        QCOMPARE(f.buffer.cellAt(4, 0).attributes, TextAttribute::Bold);
        // "-tag" at x=8..11 has span s2 (#00ff00, Underline)
        QCOMPARE(f.buffer.cellAt(8, 0).fgColor, 0xff00ff00u);
        QCOMPARE(f.buffer.cellAt(8, 0).attributes, TextAttribute::Underline);

        // Row 1 is selected: "      leaf:ok" (depth=1 => 2 spaces indent + 4 spaces leaf prefix = 6
        // spaces)
        QCOMPARE(f.line(1), QStringLiteral("leaf:ok"));
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("leaf"));
        // "leaf:" at x=6..10 has nodeFormat span 0 (#00ffff, Italic | Bold | Reversed from
        // selection)
        QCOMPARE(f.buffer.cellAt(6, 1).fgColor, 0xff00ffffu);
        QVERIFY((f.buffer.cellAt(6, 1).attributes & TextAttribute::Italic) != TextAttribute::None);
        // "ok" at x=11..12 has nodeFormat span 1 (#00ff88)
        QCOMPARE(f.buffer.cellAt(11, 1).fgColor, 0xff00ff88u);

        // Selecting row 0 derives plain text "root-tag" for display
        f.tree->setCurrentIndex(0);
        QCOMPARE(f.tree->currentItemData().value("display").toString(), QStringLiteral("root-tag"));

        // nodeFormat cache: idle repaints do not re-invoke nodeFormat, and dataChanged invalidates
        // only the modified node.
        f.engine.globalObject().setProperty("nodeFormatCalls", 0);
        f.tree->setNodeFormat(f.engine.evaluate("(function(row, val) { ++nodeFormatCalls; return "
                                                "row === 1 ? { text: val + '!' } : undefined; })"));
        f.paint();
        QCOMPARE(f.engine.globalObject().property("nodeFormatCalls").toInt(), 2);
        f.paint();
        QCOMPARE(f.engine.globalObject().property("nodeFormatCalls").toInt(), 2);
        leafItem->setText(QStringLiteral("updated"));
        f.paint();
        QCOMPARE(f.engine.globalObject().property("nodeFormatCalls").toInt(), 3);
        QCOMPARE(f.line(1), QStringLiteral("updated!"));
    }

    void delegateChooserAndPaintClipping() {
        auto model = makeTree(2, 1);
        // 1. DelegateChooser resolves distinct delegates per node role (hasChildren).
        {
            TreeFixture f;
            QVERIFY2(f.create(model.get(), QStringLiteral(R"(
                pragma ComponentBehavior: Bound
                import QtQml
                import Qt.labs.qmlmodels
                import QmlTui
                TreeView {
                    width: 40; height: 8; model: testModel
                    delegate: DelegateChooser {
                        role: "hasChildren"
                        DelegateChoice {
                            roleValue: true
                            delegate: Text {
                                required property string display
                                text: "DIR:" + display
                            }
                        }
                        DelegateChoice {
                            roleValue: false
                            delegate: Text {
                                required property string display
                                text: "FILE:" + display
                            }
                        }
                    }
                }
            )")),
                     qPrintable(f.component.errorString()));
            f.tree->expand(0);
            f.paint();
            QCOMPARE(f.line(0), QStringLiteral("DIR:group 0"));
            QCOMPARE(f.line(1), QStringLiteral("FILE:leaf 0.0"));
            QCOMPARE(f.line(2), QStringLiteral("DIR:group 1"));
        }

        // 2. Built-in TreeView::paint() strictly respects clipRect.
        {
            TreeFixture f;
            QVERIFY2(f.create(model.get(), builtinQml), qPrintable(f.component.errorString()));
            f.tree->expand(0);
            f.tree->setCurrentIndex(1);
            f.buffer.clear();
            // Fill buffer with sentinel '.' so any write outside clipRect is detected.
            f.buffer.fillRect(0, 0, f.buffer.width(), f.buffer.height(), ".", Color::Default,
                              Color::Default);
            testing::paintItem(f.tree, f.buffer, 0, 0, ClipRect{2, 1, 6, 1});
            // Row 0 (outside clipY) must stay untouched '.'.
            for (int x = 0; x < 20; ++x)
                QCOMPARE(QString::fromUtf8(f.buffer.cellAt(x, 0).character()), QStringLiteral("."));
            // Row 1 outside [2, 8) must stay untouched '.'.
            QCOMPARE(QString::fromUtf8(f.buffer.cellAt(0, 1).character()), QStringLiteral("."));
            QCOMPARE(QString::fromUtf8(f.buffer.cellAt(1, 1).character()), QStringLiteral("."));
            QCOMPARE(QString::fromUtf8(f.buffer.cellAt(8, 1).character()), QStringLiteral("."));
            // Row 1 inside [2, 8) has selection attributes and painted text ('l','e' at x=6,7).
            QCOMPARE(QString::fromUtf8(f.buffer.cellAt(6, 1).character()), QStringLiteral("l"));
            QCOMPARE(QString::fromUtf8(f.buffer.cellAt(7, 1).character()), QStringLiteral("e"));
            QVERIFY((f.buffer.cellAt(2, 1).attributes & TextAttribute::Bold) !=
                    TextAttribute::None);
            QCOMPARE(f.buffer.cellAt(1, 1).attributes, TextAttribute::None);

            // Numpad Key_Enter activates and toggles expansion just like Key_Return.
            f.tree->collapse(0);
            f.tree->setCurrentIndex(0);
            QSignalSpy actSpy(f.tree, &TreeView::itemActivated);
            KeyEvent enterEv(Qt::Key_Enter, QString(), Qt::NoModifier);
            f.tree->keyPressEvent(&enterEv);
            QVERIFY(enterEv.isAccepted());
            QVERIFY(f.tree->isExpanded(0));
            QCOMPARE(actSpy.count(), 1);
        }
    }
};

QTEST_MAIN(TestTreeView)
#include "test_tree_view.moc"
