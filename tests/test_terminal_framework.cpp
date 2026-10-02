#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QAbstractListModel>
#include <QSignalSpy>
#include <QCoreApplication>
#include <memory>

#include "UnicodeText.h"
#include "TextGridBuffer.h"
#include "Terminal.h"
#include "Popup.h"
#include "TextArea.h"
#include "LogView.h"
#include "LinearLayout.h"
#include "Layout.h"
#include "KeyEvent.h"
#include "TreeView.h"

#define private public
#include "TerminalEngine.h"
#undef private

using namespace qtui;

class MutableListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum { NameRole = Qt::UserRole + 1 };

    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : m_values.size();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || index.row() >= m_values.size()) return {};
        return role == Qt::DisplayRole || role == NameRole ? m_values[index.row()] : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::DisplayRole, "display"}, {NameRole, "name"}};
    }
    void append(const QString &value) {
        const int row = m_values.size();
        beginInsertRows({}, row, row);
        m_values.append(value);
        endInsertRows();
    }
    void renameFirst(const QString &value) {
        m_values[0] = value;
        emit dataChanged(index(0), index(0), {Qt::DisplayRole, NameRole});
    }
    void moveFirstToEnd() {
        if (m_values.size() < 2) return;
        beginMoveRows({}, 0, 0, {}, m_values.size());
        m_values.move(0, m_values.size() - 1);
        endMoveRows();
    }

private:
    QStringList m_values;
};

class TestTerminalFramework : public QObject {
    Q_OBJECT

private slots:
    void unicodeDisplayWidth();
    void wideCellBuffer();
    void transparentTextPreservesBackground();
    void disabledAndModalFocus();
    void nestedModalFocus();
    void dialogDefaultAndCancelActions();
    void graphemeSelection();
    void mouseDragSelection();
    void responsiveLayout();
    void layoutInvalidation();
    void boundedLog();
    void osc52Encoding();
    void treeDelegateIdentity();
    void destroyedTreeModelIsCleared();
};

void TestTerminalFramework::unicodeDisplayWidth() {
    QCOMPARE(UnicodeText::displayWidth(QStringLiteral("ASCII")), 5);
    QCOMPARE(UnicodeText::displayWidth(QString::fromUtf8("e\u0301")), 1);
    QCOMPARE(UnicodeText::displayWidth(QString::fromUtf8("中")), 2);
    QCOMPARE(UnicodeText::displayWidth(QString::fromUtf8("👨‍👩‍👧‍👦")), 2);
    QCOMPARE(UnicodeText::graphemes(QString::fromUtf8("👨‍👩‍👧‍👦")).size(), 1);
}

void TestTerminalFramework::wideCellBuffer() {
    TextGridBuffer buffer;
    buffer.resize(5, 1);
    buffer.writeText(0, 0, QString::fromUtf8("中A"));
    QCOMPARE(buffer.cellAt(0, 0).displayWidth, 2);
    QVERIFY(buffer.cellAt(1, 0).continuation);
    QCOMPARE(buffer.cellAt(2, 0).character, QByteArray("A"));
    QVERIFY(QString::fromStdString(buffer.toAnsiString()).contains(QString::fromUtf8("中A")));
}

void TestTerminalFramework::transparentTextPreservesBackground() {
    TextGridBuffer buffer;
    buffer.resize(4, 1);
    buffer.fillRect(0, 0, 4, 1, " ", 0xFFFFFFFF, 0xFFE8E0D5);
    buffer.writeText(0, 0, QString::fromUtf8("A中"), 0xFF000000, 0x00000000);
    QCOMPARE(buffer.cellAt(0, 0).bgColor, 0xFFE8E0D5u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0xFFE8E0D5u);
    QCOMPARE(buffer.cellAt(2, 0).bgColor, 0xFFE8E0D5u);
}

void TestTerminalFramework::disabledAndModalFocus() {
    Terminal terminal;
    terminal.setWidth(40);
    terminal.setHeight(10);
    Item first(&terminal);
    Item second(&terminal);
    first.setFocusable(true);
    second.setFocusable(true);
    first.setEnabled(false);

    TerminalEngine engine(&terminal);
    engine.handleKeyInput(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &second);

    Popup popup(&terminal);
    popup.setModal(true);
    Item dialogFirst(&popup);
    Item dialogSecond(&popup);
    dialogFirst.setFocusable(true);
    dialogSecond.setFocusable(true);
    popup.open();
    QCOMPARE(terminal.focusedItem(), &dialogFirst);
    engine.handleKeyInput(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal.focusedItem(), &dialogSecond);
    engine.handleKeyInput(Qt::Key_Backtab, {}, Qt::ShiftModifier);
    QCOMPARE(terminal.focusedItem(), &dialogFirst);
    popup.close();
    QCOMPARE(terminal.focusedItem(), &second);
}

void TestTerminalFramework::nestedModalFocus() {
    Terminal terminal;
    terminal.setWidth(40);
    terminal.setHeight(10);
    Item background(&terminal);
    background.setFocusable(true);
    terminal.setFocusedItem(&background);

    Popup first(&terminal);
    first.setModal(true);
    Item firstField(&first);
    firstField.setFocusable(true);
    first.open();

    Popup second(&terminal);
    second.setModal(true);
    Item secondField(&second);
    secondField.setFocusable(true);
    second.open();
    QCOMPARE(terminal.focusedItem(), &secondField);
    second.close();
    QCOMPARE(terminal.focusedItem(), &firstField);
    first.close();
    QCOMPARE(terminal.focusedItem(), &background);
}

void TestTerminalFramework::dialogDefaultAndCancelActions() {
    QQmlEngine qmlEngine;
    qmlEngine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    qmlEngine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));
    QQmlComponent component(&qmlEngine);
    component.setData(R"(
        import Qtui
        import Qtui.Controls
        Terminal {
            width: 40
            height: 12
            ConfirmationDialog {
                objectName: "dialog"
                title: "Confirm"
                message: "Continue?"
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *terminal = qobject_cast<Terminal*>(object.get());
    auto *dialog = terminal ? terminal->findChild<Popup*>(QStringLiteral("dialog")) : nullptr;
    QVERIFY(dialog);
    TerminalEngine engine(terminal);

    QSignalSpy acceptedSpy(dialog, &Popup::accepted);
    dialog->open();
    engine.handleKeyInput(Qt::Key_Return, QStringLiteral("\n"), 0);
    QCOMPARE(acceptedSpy.count(), 1);
    QVERIFY(!dialog->visible());

    QSignalSpy rejectedSpy(dialog, &Popup::rejected);
    dialog->open();
    engine.handleKeyInput(Qt::Key_Escape, {}, 0);
    QCOMPARE(rejectedSpy.count(), 1);
    QVERIFY(!dialog->visible());
}

void TestTerminalFramework::graphemeSelection() {
    Terminal terminal;
    TextArea area(&terminal);
    const QString family = QString::fromUtf8("👨‍👩‍👧‍👦");
    area.setText(family + QStringLiteral("x"));
    terminal.setFocusedItem(&area);
    KeyEvent selectRight(Qt::Key_Right, {}, Qt::ShiftModifier);
    area.keyPressEvent(&selectRight);
    QCOMPARE(area.selectedText(), family);
}

void TestTerminalFramework::mouseDragSelection() {
    Terminal terminal;
    terminal.setWidth(20);
    terminal.setHeight(4);
    TextArea area(&terminal);
    area.setWidth(10);
    area.setHeight(2);
    area.setText(QStringLiteral("abcdef"));
    TerminalEngine engine(&terminal);
    engine.handleMouseInput(0, 0, 0, false);
    engine.handleMouseInput(0, 3, 0, false, true);
    engine.handleMouseInput(0, 3, 0, true);
    QCOMPARE(area.selectedText(), QStringLiteral("abc"));
}

void TestTerminalFramework::responsiveLayout() {
    RowLayout layout;
    layout.setWidth(10);
    layout.setHeight(2);
    Item first(&layout);
    Item second(&layout);
    first.setImplicitWidth(2);
    second.setImplicitWidth(2);
    auto *firstInfo = qobject_cast<LayoutAttached*>(
        qmlAttachedPropertiesObject<Layout>(&first, true));
    auto *secondInfo = qobject_cast<LayoutAttached*>(
        qmlAttachedPropertiesObject<Layout>(&second, true));
    firstInfo->fillWidth = true;
    secondInfo->fillWidth = true;
    TextGridBuffer buffer;
    buffer.resize(10, 2);
    layout.render(buffer, 0, 0);
    QCOMPARE(first.width() + second.width(), 10);
    QCOMPARE(second.x(), first.width());
}

void TestTerminalFramework::layoutInvalidation() {
    RowLayout layout;
    layout.setWidth(3);
    layout.setHeight(1);
    Item child(&layout);
    child.setImplicitWidth(1);
    auto *info = qobject_cast<LayoutAttached*>(
        qmlAttachedPropertiesObject<Layout>(&child, true));
    QSignalSpy dirtySpy(&layout, &Item::dirty);
    QVERIFY(info->setProperty("minimumWidth", 5));
    QVERIFY(dirtySpy.count() > 0);

    TextGridBuffer buffer;
    buffer.resize(3, 1);
    layout.render(buffer, 0, 0);
    QCOMPARE(child.width(), 5);
    QVERIFY(layout.clip());
}

void TestTerminalFramework::boundedLog() {
    LogView log;
    log.setMaximumLineCount(2);
    log.appendLine(QStringLiteral("one"));
    log.appendLine(QStringLiteral("two"));
    log.appendLine(QStringLiteral("three"));
    QCOMPARE(log.text(), QStringLiteral("two\nthree"));
}

void TestTerminalFramework::osc52Encoding() {
    QCOMPARE(TerminalEngine::osc52Sequence(QStringLiteral("hello")),
             std::string("\033]52;c;aGVsbG8=\a"));
}

void TestTerminalFramework::treeDelegateIdentity() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../Qtui"));
    MutableListModel model;
    model.append(QStringLiteral("first"));
    model.append(QStringLiteral("second"));
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        import Qtui.Widgets
        TreeView {
            width: 30
            height: 5
            model: testModel
            delegate: Item {
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool isCurrent
                required property string display
                required property var modelData
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *tree = qobject_cast<TreeView*>(object.get());
    QVERIFY(tree);
    TextGridBuffer buffer;
    buffer.resize(30, 5);
    tree->render(buffer, 0, 0);
    const QList<Item*> initialItems = tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(!initialItems.isEmpty());
    Item *firstDelegate = initialItems.first();

    model.renameFirst(QStringLiteral("renamed"));
    tree->render(buffer, 0, 0);
    QVERIFY(tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly).contains(firstDelegate));

    model.append(QStringLiteral("third"));
    tree->render(buffer, 0, 0);
    QVERIFY(tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly).contains(firstDelegate));

    model.moveFirstToEnd();
    tree->render(buffer, 0, 0);
    Item *firstVisible = nullptr;
    const QList<Item*> delegates = tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
    for (Item *delegate : delegates) {
        if (delegate->y() == 0) {
            firstVisible = delegate;
            break;
        }
    }
    QVERIFY(firstVisible);
    QCOMPARE(firstVisible->property("display").toString(), QStringLiteral("second"));

    tree->setHeight(1);
    tree->setCurrentIndex(2);
    tree->render(buffer, 0, 0);
    bool selectedDelegateWasMaterialized = false;
    for (Item *delegate : tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (delegate->property("index").toInt() == 2 && delegate->visible()) {
            selectedDelegateWasMaterialized = true;
            break;
        }
    }
    QVERIFY(selectedDelegateWasMaterialized);
}

void TestTerminalFramework::destroyedTreeModelIsCleared() {
    TreeView tree;
    auto *model = new MutableListModel;
    model->append(QStringLiteral("row"));
    tree.setModel(QVariant::fromValue(static_cast<QObject*>(model)));
    QVERIFY(tree.model().isValid());

    delete model;
    QVERIFY(!tree.model().isValid());
}

QTEST_MAIN(TestTerminalFramework)
#include "test_terminal_framework.moc"
