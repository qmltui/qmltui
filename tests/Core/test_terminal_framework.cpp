#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QAbstractListModel>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QCoreApplication>
#include <QtLogging>
#include <algorithm>
#include <memory>

#include <Qamui/Core/Primitives/UnicodeText.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>
#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Core/Items/Popup.h>
#include <Qamui/Core/Items/MouseArea.h>
#include <Qamui/Core/Items/TextArea.h>
#include <Qamui/Core/Items/Text.h>
#include <Qamui/Core/Engine/State.h>
#include <Qamui/Core/Items/LogView.h>
#include <Qamui/Core/Items/Scrollback.h>
#include <Qamui/Core/Items/LinearLayout.h>
#include <Qamui/Core/Items/Layout.h>
#include <Qamui/Core/Engine/KeyEvent.h>
#include <Qamui/Core/Items/ListView.h>
#include <Qamui/Core/Items/TreeView.h>
#include "../TestPaint.h"

#include <Qamui/Core/Engine/TerminalEngine.h>
#include <Qamui/Core/Engine/QtMessageRouting.h>
#undef private

using namespace qamui;
using qamui::testing::paintItem;

namespace {

QStringList* capturedMessages = nullptr;

void captureMessage(QtMsgType, const QMessageLogContext&, const QString& message)
{
    if (capturedMessages)
        capturedMessages->append(message);
}

class ScopedMessageCapture
{
public:
    ScopedMessageCapture()
        : m_previousHandler(qInstallMessageHandler(captureMessage))
    {
        Q_ASSERT(!capturedMessages);
        capturedMessages = &messages;
    }

    ~ScopedMessageCapture()
    {
        qInstallMessageHandler(m_previousHandler);
        capturedMessages = nullptr;
    }

    QStringList messages;

private:
    QtMessageHandler m_previousHandler;
};

} // namespace

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
    void insertAt(int row, const QString &value) {
        beginInsertRows({}, row, row);
        m_values.insert(row, value);
        endInsertRows();
    }
    void removeAt(int row) {
        beginRemoveRows({}, row, row);
        m_values.removeAt(row);
        endRemoveRows();
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

class DelegateRecorder : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE void record(int index, const QVariant &model,
                            const QVariant &modelData, const QString &display)
    {
        indexes.append(index);
        displays.append(display);
        modelValues.append(model.toMap().value(QStringLiteral("display")).toString());
        modelDataValues.append(
            modelData.toMap().value(QStringLiteral("display")).toString());
    }

    QList<int> indexes;
    QStringList displays;
    QStringList modelValues;
    QStringList modelDataValues;
};

class InstanceTracker : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE void instanceCreated() { ++createdCount; }
    Q_INVOKABLE void itemRendered(const QString &val) { renderedItems.append(val); }

    int createdCount = 0;
    QStringList renderedItems;
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
    void textAreaNavigationBounds();
    void reentrantStateChange();
    void mouseMotionIsDemandDriven();
    void mouseDragSelection();
    void responsiveLayout();
    void layoutInvalidation();
    void boundedLog();
    void osc52Encoding();
    void treeDelegateIdentity();
    void listModelMutationContract();
    void listViewVirtualizesLargeModels();
    void scrollbackDelegateContract();
    void scrollbackDelegateReuse();
    void scrollbackSpacingAndSeparator();
    void scrollbackImperativeMethods();
    void terminalVisibleChildrenAndAutoImplicitHeight();
    void destroyedTreeModelIsCleared();
    void keyHandlingImprovements();
    void alternateScreenDefersMessages();
    void nestedAlternateScreensDelayFlush();
    void normalScreenForwardsMessagesImmediately();
    void testSessionOutputModeTransitions();
    void terminalTitle();
    void terminalBackgroundColor();
};

void TestTerminalFramework::testSessionOutputModeTransitions() {
    const QByteArray previousScript = qgetenv("QAMUI_RUNNER_SCRIPT");
    const QByteArray previousLive = qgetenv("QAMUI_RUNNER_LIVE");
    qputenv("QAMUI_RUNNER_SCRIPT", QByteArrayLiteral("quiet-test.js"));
    qunsetenv("QAMUI_RUNNER_LIVE");

    bool quietByDefault = false;
    bool enabledOnRequest = false;
    bool quietAgain = false;
    {
        Terminal terminal;
        TerminalEngine engine(&terminal);
        quietByDefault = !engine.terminalOutputEnabled();
        engine.setTerminalOutputEnabled(true);
        enabledOnRequest = engine.terminalOutputEnabled();
        engine.setTerminalOutputEnabled(false);
        quietAgain = !engine.terminalOutputEnabled();
    }

    if (previousScript.isNull())
        qunsetenv("QAMUI_RUNNER_SCRIPT");
    else
        qputenv("QAMUI_RUNNER_SCRIPT", previousScript);
    if (previousLive.isNull())
        qunsetenv("QAMUI_RUNNER_LIVE");
    else
        qputenv("QAMUI_RUNNER_LIVE", previousLive);

    QVERIFY(quietByDefault);
    QVERIFY(enabledOnRequest);
    QVERIFY(quietAgain);
}

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
    QCOMPARE(buffer.cellAt(2, 0).character().toByteArray(), QByteArray("A"));
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
    terminal.engine()->setTestViewportSize(QSize(40, 10));
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
    terminal.engine()->setTestViewportSize(QSize(40, 10));
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
    QQmlComponent component(&qmlEngine);
    component.setData(R"(
        import Qamui
        import Qamui.Controls
        Terminal {
            width: 40
            height: 12
            Dialog {
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

void TestTerminalFramework::textAreaNavigationBounds() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(10, 4));
    TextArea area(&terminal);
    area.setWidth(10);
    area.setHeight(4);
    area.setText(QStringLiteral("abc\ndef"));
    terminal.setFocusedItem(&area);
    for (const int key : {Qt::Key_Up, Qt::Key_PageUp}) {
        area.setCursorPosition(1);
        KeyEvent event(key, {});
        area.keyPressEvent(&event);
        QCOMPARE(area.cursorPosition(), 1);
        terminal.engine()->renderFrame();
        const auto frame = terminal.engine()->currentFrame();
        QVERIFY(frame);
        QVERIFY((frame->cellAt(1, 0).attributes & TextAttribute::Reversed) != TextAttribute::None);
    }
    for (const int key : {Qt::Key_Down, Qt::Key_PageDown}) {
        area.setCursorPosition(5);
        KeyEvent event(key, {});
        area.keyPressEvent(&event);
        QCOMPARE(area.cursorPosition(), 5);
    }
    area.setCursorPosition(1);
    KeyEvent selectUp(Qt::Key_Up, {}, Qt::ShiftModifier);
    area.keyPressEvent(&selectUp);
    QVERIFY(area.selectedText().isEmpty());
}

void TestTerminalFramework::reentrantStateChange() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(12, 2));
    Text label(&terminal);
    label.setText(QStringLiteral("idle"));
    for (const QString name : {QStringLiteral("loading"), QStringLiteral("ready")}) {
        auto *state = new State(&label);
        state->setName(name);
        auto *change = new PropertyChanges(state);
        change->setTarget(&label);
        change->setProperties({{QStringLiteral("text"), name}});
        auto changes = state->changes();
        changes.append(&changes, change);
        auto states = label.states();
        states.append(&states, state);
    }
    const auto redirect = connect(&label, &Item::stateChanged, &label, [&] {
        if (label.state() == QStringLiteral("loading"))
            label.setState(QStringLiteral("ready"));
    });
    label.setState(QStringLiteral("loading"));
    QCOMPARE(label.state(), QStringLiteral("ready"));
    QCOMPARE(label.text(), QStringLiteral("ready"));
    terminal.engine()->renderFrame();
    const auto frame = terminal.engine()->currentFrame();
    QVERIFY(frame);
    QCOMPARE(frame->cellAt(0, 0).character(), QByteArrayView("r"));
    label.setState({});
    QCOMPARE(label.text(), QStringLiteral("idle"));
    disconnect(redirect);

    // An empty pending state is a real request, and the latest request wins.
    const auto reset = connect(&label, &Item::stateChanged, &label, [&] {
        if (label.state() == QStringLiteral("loading")) {
            label.setState(QStringLiteral("ready"));
            label.setState({});
        }
    });
    label.setState(QStringLiteral("loading"));
    QCOMPARE(label.state(), QString());
    QCOMPARE(label.text(), QStringLiteral("idle"));
    disconnect(reset);

    // Property application can also trigger State.when reevaluation.
    auto states = label.states();
    State *ready = states.at(&states, 1);
    connect(&label, &Text::textChanged, &label, [&] {
        if (label.text() == QStringLiteral("loading"))
            ready->setWhen(true);
    });
    label.setState(QStringLiteral("loading"));
    QCOMPARE(label.state(), QStringLiteral("ready"));
    QCOMPARE(label.text(), QStringLiteral("ready"));
}

void TestTerminalFramework::mouseDragSelection() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 4));
    TextArea area(&terminal);
    area.setWidth(10);
    area.setHeight(2);
    area.setText(QStringLiteral("abcdef"));
    TerminalEngine engine(&terminal);
    engine.handleMouseInput(0, 0, 0, false);
    engine.handleMouseInput(3, 5, 0, false, true);
    QVERIFY(area.selectedText().isEmpty());
    engine.handleMouseInput(0, 3, 0, false, true);
    engine.handleMouseInput(0, 3, 0, true);
    QCOMPARE(area.selectedText(), QStringLiteral("abc"));
}

void TestTerminalFramework::mouseMotionIsDemandDriven() {
    Terminal terminal;
    terminal.engine()->setTestViewportSize(QSize(20, 4));
    TerminalEngine engine(&terminal);
    MouseArea first(&terminal);
    first.setWidth(10);
    first.setHeight(2);
    MouseArea second(&terminal);

    QVERIFY(!terminal.hasMouseMotionRequests());
    QVERIFY(!engine.m_mouseMotionReportingEnabled);

    engine.handleMouseInput(3, 4, 1, false, true);
    QCOMPARE(first.mouseX(), 0);
    QCOMPARE(first.mouseY(), 0);

    first.setHoverEnabled(true);
    QVERIFY(terminal.hasMouseMotionRequests());
    QVERIFY(engine.m_mouseMotionReportingEnabled);
    engine.handleMouseInput(3, 4, 1, false, true);
    QCOMPARE(first.mouseX(), 4);
    QCOMPARE(first.mouseY(), 1);

    second.setHoverEnabled(true);
    first.setHoverEnabled(false);
    QVERIFY(terminal.hasMouseMotionRequests());
    QVERIFY(engine.m_mouseMotionReportingEnabled);
    second.setHoverEnabled(false);
    QVERIFY(!terminal.hasMouseMotionRequests());
    QVERIFY(!engine.m_mouseMotionReportingEnabled);

    auto* transient = new MouseArea(&terminal);
    transient->setHoverEnabled(true);
    QVERIFY(terminal.hasMouseMotionRequests());
    delete transient;
    QVERIFY(!terminal.hasMouseMotionRequests());

    Terminal otherTerminal;
    auto* reparented = new MouseArea(&terminal);
    reparented->setHoverEnabled(true);
    reparented->setParentItem(&otherTerminal);
    QVERIFY(!terminal.hasMouseMotionRequests());
    QVERIFY(otherTerminal.hasMouseMotionRequests());
    reparented->setHoverEnabled(false);
    delete reparented;
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
    paintItem(layout, buffer, 0, 0);
    QCOMPARE(first.width() + second.width(), 10);
    QCOMPARE(first.width(), 5);
    QCOMPARE(second.width(), 5);
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
    paintItem(layout, buffer, 0, 0);
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
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    MutableListModel model;
    model.append(QStringLiteral("first"));
    model.append(QStringLiteral("second"));
    engine.rootContext()->setContextProperty("testModel", &model);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qamui
        
        TreeView {
            width: 30
            height: 5
            model: testModel
            delegate: Item {
                required property int depth
                required property bool hasChildren
                required property bool expanded
                required property bool current
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
    paintItem(tree, buffer, 0, 0);
    const QList<Item*> initialItems = tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(!initialItems.isEmpty());
    Item *firstDelegate = initialItems.first();

    model.renameFirst(QStringLiteral("renamed"));
    paintItem(tree, buffer, 0, 0);
    QVERIFY(tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly).contains(firstDelegate));

    model.append(QStringLiteral("third"));
    paintItem(tree, buffer, 0, 0);
    QVERIFY(tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly).contains(firstDelegate));

    model.moveFirstToEnd();
    paintItem(tree, buffer, 0, 0);
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
    paintItem(tree, buffer, 0, 0);
    bool selectedDelegateWasMaterialized = false;
    for (Item *delegate : tree->findChildren<Item*>(QString(), Qt::FindDirectChildrenOnly)) {
        if (delegate->property("index").toInt() == 2 && delegate->visible()) {
            selectedDelegateWasMaterialized = true;
            break;
        }
    }
    QVERIFY(selectedDelegateWasMaterialized);
}

void TestTerminalFramework::listModelMutationContract() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    QQmlComponent delegate(&engine);
    delegate.setData(R"(
        import Qamui
        Item {
            required property int index
            required property string display
            required property var model
            required property var modelData
            property bool attachedCurrent: ListView.isCurrentItem
        }
    )", QUrl());
    QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));

    MutableListModel model;
    model.append(QStringLiteral("first"));
    model.append(QStringLiteral("second"));
    model.append(QStringLiteral("third"));

    ListView list;
    QQmlEngine::setContextForObject(&list, engine.rootContext());
    list.setWidth(30);
    list.setHeight(5);
    list.setDelegate(&delegate);
    list.setModel(QVariant::fromValue(static_cast<QObject *>(&model)));
    QCOMPARE(list.currentIndex(), -1);

    list.setCurrentIndex(1);
    QCOMPARE(list.currentIndex(), 1);
    model.insertAt(0, QStringLiteral("inserted"));
    QCOMPARE(list.currentIndex(), 2);
    model.removeAt(0);
    QCOMPARE(list.currentIndex(), 1);
    model.moveFirstToEnd();
    QCOMPARE(list.currentIndex(), 0);

    Item *currentDelegate = nullptr;
    for (Item *item : list.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (item->property("attachedCurrent").toBool()) {
            currentDelegate = item;
            break;
        }
    }
    QVERIFY(currentDelegate);
    QCOMPARE(currentDelegate->property("index").toInt(), 0);
    QCOMPARE(currentDelegate->property("display").toString(), QStringLiteral("second"));

    model.removeAt(0);
    QCOMPARE(list.currentIndex(), 0);
    list.setCurrentIndex(-1);
    QCOMPARE(list.currentIndex(), -1);
}

void TestTerminalFramework::listViewVirtualizesLargeModels() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    QQmlComponent delegate(&engine);
    delegate.setData(R"(
        import Qamui
        Item {
            required property int index
            required property string display
            height: 2
        }
    )", QUrl());
    QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));

    MutableListModel model;
    for (int row = 0; row < 500; ++row)
        model.append(QStringLiteral("row-%1").arg(row));

    ListView list;
    QQmlEngine::setContextForObject(&list, engine.rootContext());
    list.setWidth(30);
    list.setHeight(5);
    list.setDelegate(&delegate);
    list.setModel(QVariant::fromValue(static_cast<QObject *>(&model)));

    TextGridBuffer buffer;
    buffer.resize(30, 5);
    paintItem(list, buffer, 0, 0);
    QCOMPARE(list.contentHeight(), 1000);
    QVERIFY(list.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly).size() <= 7);

    list.setContentY(400);
    paintItem(list, buffer, 0, 0);
    const QList<Item *> scrolledDelegates =
        list.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(scrolledDelegates.size() <= 7);
    QVERIFY(std::any_of(scrolledDelegates.cbegin(), scrolledDelegates.cend(),
                        [](Item *item) { return item->property("index").toInt() >= 200; }));

    list.setCurrentIndex(450);
    paintItem(list, buffer, 0, 0);
    QCOMPARE(list.currentIndex(), 450);
    QVERIFY(list.contentY() > 800);
    bool currentWasMaterialized = false;
    for (Item *item : list.findChildren<Item *>(QString(), Qt::FindDirectChildrenOnly)) {
        if (item->property("index").toInt() == 450) {
            currentWasMaterialized = true;
            break;
        }
    }
    QVERIFY(currentWasMaterialized);
}

void TestTerminalFramework::scrollbackDelegateContract() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    DelegateRecorder recorder;
    engine.rootContext()->setContextProperty(QStringLiteral("delegateRecorder"), &recorder);

    QQmlComponent invalidDelegate(&engine);
    invalidDelegate.setData(R"(
        import QtQml
        QtObject {}
    )", QUrl());
    QVERIFY2(invalidDelegate.isReady(), qPrintable(invalidDelegate.errorString()));

    QQmlComponent validDelegate(&engine);
    validDelegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            required property int index
            required property var model
            required property var modelData
            required property string display
            property bool completed: false
            Component.onCompleted: {
                completed = true;
                delegateRecorder.record(index, model, modelData, display);
            }
            onDisplayChanged: {
                if (completed)
                    delegateRecorder.record(index, model, modelData, display);
            }
        }
    )", QUrl());
    QVERIFY2(validDelegate.isReady(), qPrintable(validDelegate.errorString()));

    Terminal terminal;
    Scrollback scrollback(&terminal);
    QQmlEngine::setContextForObject(&scrollback, engine.rootContext());
    MutableListModel model;
    model.append(QStringLiteral("first"));
    model.append(QStringLiteral("second"));

    QTest::ignoreMessage(
        QtWarningMsg,
        QRegularExpression(QStringLiteral(".*Delegate root must derive from Qamui.Item.*")));
    scrollback.setDelegate(&invalidDelegate);
    scrollback.setModel(&model);
    QVERIFY(recorder.indexes.isEmpty());

    scrollback.setDelegate(&validDelegate);
    QCOMPARE(recorder.indexes, QList<int>({0, 1}));
    QCOMPARE(recorder.displays,
             QStringList({QStringLiteral("first"), QStringLiteral("second")}));
    QCOMPARE(recorder.modelValues, recorder.displays);
    QCOMPARE(recorder.modelDataValues, recorder.displays);
}

void TestTerminalFramework::scrollbackDelegateReuse() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    InstanceTracker tracker;
    engine.rootContext()->setContextProperty(QStringLiteral("tracker"), &tracker);

    QQmlComponent delegate(&engine);
    delegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            required property int index
            required property string display
            property bool ready: false
            Component.onCompleted: {
                tracker.instanceCreated();
                ready = true;
                tracker.itemRendered(display);
            }
            onDisplayChanged: {
                if (ready)
                    tracker.itemRendered(display);
            }
        }
    )", QUrl());
    QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));

    Terminal terminal;
    Scrollback scrollback(&terminal);
    QQmlEngine::setContextForObject(&scrollback, engine.rootContext());
    scrollback.setReuseItems(true);
    scrollback.setDelegate(&delegate);

    MutableListModel model;
    model.append(QStringLiteral("item1"));
    model.append(QStringLiteral("item2"));
    model.append(QStringLiteral("item3"));

    scrollback.setModel(&model);

    QCOMPARE(tracker.createdCount, 1);
    QCOMPARE(tracker.renderedItems, QStringList({QStringLiteral("item1"), QStringLiteral("item2"), QStringLiteral("item3")}));

    tracker.createdCount = 0;
    tracker.renderedItems.clear();

    Scrollback nonReusedView(&terminal);
    QQmlEngine::setContextForObject(&nonReusedView, engine.rootContext());
    nonReusedView.setReuseItems(false);
    nonReusedView.setDelegate(&delegate);

    MutableListModel model2;
    model2.append(QStringLiteral("a"));
    model2.append(QStringLiteral("b"));
    nonReusedView.setModel(&model2);

    QCOMPARE(tracker.createdCount, 2);
    QCOMPARE(tracker.renderedItems, QStringList({QStringLiteral("a"), QStringLiteral("b")}));
}

void TestTerminalFramework::scrollbackSpacingAndSeparator() {
    // A quiet test-session engine records the rows it commits.
    const QByteArray previousScript = qgetenv("QAMUI_RUNNER_SCRIPT");
    qputenv("QAMUI_RUNNER_SCRIPT", QByteArrayLiteral("quiet-test.js"));
    const auto restoreEnvironment = qScopeGuard([&] {
        if (previousScript.isEmpty())
            qunsetenv("QAMUI_RUNNER_SCRIPT");
        else
            qputenv("QAMUI_RUNNER_SCRIPT", previousScript);
    });

    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    QQmlComponent itemDelegate(&engine);
    itemDelegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            width: 5
            height: 1
            required property string display
            Text { text: display }
        }
    )", QUrl());
    QVERIFY2(itemDelegate.isReady(), qPrintable(itemDelegate.errorString()));

    QQmlComponent sepDelegate(&engine);
    sepDelegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            width: 5
            height: 1
            Text { text: "-----" }
        }
    )", QUrl());
    QVERIFY2(sepDelegate.isReady(), qPrintable(sepDelegate.errorString()));

    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    auto* terminalEngine = terminal.engine();
    terminalEngine->setTestViewportSize(QSize(20, 6));
    QVERIFY(terminalEngine->testSession());
    QVERIFY(terminalEngine->scrollbackHistoryEnabled());
    Scrollback scrollback(&terminal);
    QQmlEngine::setContextForObject(&scrollback, engine.rootContext());
    scrollback.setDelegate(&itemDelegate);
    scrollback.setSeparator(&sepDelegate);
    scrollback.setSpacing(1);

    MutableListModel model;
    model.append(QStringLiteral("A"));
    model.append(QStringLiteral("B"));
    scrollback.setModel(&model);

    QQmlComponent overrideDelegate(&engine);
    overrideDelegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            width: 5
            height: 1
            property int streamSpacing: 0
            required property string display
            Text { text: display }
        }
    )", QUrl());
    QVERIFY2(overrideDelegate.isReady(), qPrintable(overrideDelegate.errorString()));

    Scrollback overrideScrollback(&terminal);
    QQmlEngine::setContextForObject(&overrideScrollback, engine.rootContext());
    overrideScrollback.setDelegate(&overrideDelegate);
    overrideScrollback.setSpacing(3);

    MutableListModel model3;
    model3.append(QStringLiteral("X"));
    model3.append(QStringLiteral("Y"));
    overrideScrollback.setModel(&model3);

    // One stream render commits everything both objects submitted: item,
    // separator, one spacing row, item; then two items whose streamSpacing
    // override suppresses the spacing.
    terminalEngine->renderFrame();
    const TextGridBuffer rows = terminalEngine->committedScrollback();
    const auto rowText = [&](int y) {
        QString text;
        for (int x = 0; x < rows.width(); ++x) {
            const Cell& cell = rows.cellAt(x, y);
            if (cell.continuation) continue;
            text += cell.character().isEmpty() ? QStringLiteral(" ") : QString::fromUtf8(cell.character());
        }
        while (text.endsWith(QLatin1Char(' '))) text.chop(1);
        return text;
    };
    QCOMPARE(rows.height(), 6);
    QCOMPARE(rowText(0), QStringLiteral("A"));
    QCOMPARE(rowText(1), QStringLiteral("-----"));
    QCOMPARE(rowText(2), QString());
    QCOMPARE(rowText(3), QStringLiteral("B"));
    QCOMPARE(rowText(4), QStringLiteral("X"));
    QCOMPARE(rowText(5), QStringLiteral("Y"));

    scrollback.clear();
    QCOMPARE(scrollback.model().isValid(), true);
}

void TestTerminalFramework::scrollbackImperativeMethods() {
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));

    QQmlComponent delegate(&engine);
    delegate.setData(R"(
        import QtQml
        import Qamui
        Item {
            width: 10
            height: 1
            required property string text
            Text { text: text }
        }
    )", QUrl());
    QVERIFY2(delegate.isReady(), qPrintable(delegate.errorString()));

    Terminal terminal;
    Scrollback scrollback(&terminal);
    QQmlEngine::setContextForObject(&scrollback, engine.rootContext());
    scrollback.setDelegate(&delegate);

    QVariantMap data;
    data.insert(QStringLiteral("text"), QStringLiteral("hello"));
    scrollback.append(data);

    scrollback.print(QStringLiteral("direct-ansi\n"));

    Item customItem;
    customItem.setWidth(8);
    customItem.setHeight(1);
    scrollback.append(&customItem);
}

void TestTerminalFramework::terminalVisibleChildrenAndAutoImplicitHeight() {
    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);

    QCOMPARE(terminal.implicitHeight(), 0);

    Item child1;
    child1.setHeight(3);
    child1.setParentItem(&terminal);

    QCOMPARE(terminal.implicitHeight(), 3);

    Item child2;
    child2.setY(2);
    child2.setHeight(5);
    child2.setParentItem(&terminal);

    QCOMPARE(terminal.implicitHeight(), 7); // 2 + 5 = 7

    child2.setVisible(false);
    QCOMPARE(terminal.implicitHeight(), 3); // child2 is hidden

    // Ordinary visible children contribute to inline height
    Item liveWidget;
    liveWidget.setHeight(4);
    liveWidget.setParentItem(&terminal);

    QCOMPARE(terminal.implicitHeight(), 4);

    liveWidget.setHeight(6);
    QCOMPARE(terminal.implicitHeight(), 6);

    // Explicit override takes precedence if positive
    terminal.setImplicitHeight(10);
    QCOMPARE(terminal.implicitHeight(), 10);
    terminal.setImplicitHeight(0);
    QCOMPARE(terminal.implicitHeight(), 6);
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

void TestTerminalFramework::keyHandlingImprovements() {
    QQmlEngine qmlEngine;
    qmlEngine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    QQmlComponent component(&qmlEngine);
    component.setData(R"(
        import Qamui
        Terminal {
            id: root
            width: 40
            height: 12
            focus: true

            property bool returnPressedFired: false
            property bool escapePressedFired: false
            property bool tabPressedFired: false
            property bool genericPressedFired: false
            property int lastGenericKey: 0

            property bool childPressedFired: false
            property bool shortcutFired: false
            property bool textInputLeftFired: false

            Keys.priority: Keys.BeforeItem
            Keys.forwardTo: [childItem]

            Keys.onReturnPressed: (event) => {
                returnPressedFired = true;
                event.accepted = true;
            }
            Keys.onEscapePressed: (event) => {
                escapePressedFired = true;
                event.accepted = true;
            }
            Keys.onTabPressed: (event) => {
                tabPressedFired = true;
                event.accepted = true;
            }
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_A) {
                    genericPressedFired = true;
                    lastGenericKey = event.key;
                    event.accepted = true;
                }
            }

            Item {
                id: childItem
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_B) {
                        root.childPressedFired = true;
                        event.accepted = true;
                    }
                }
            }

            TextInput {
                id: textInputBefore
                objectName: "textInputBefore"
                Keys.priority: Keys.BeforeItem
                Keys.onLeftPressed: (event) => {
                    root.textInputLeftFired = true;
                    event.accepted = true;
                }
            }

            TextInput {
                id: textInputAfter
                objectName: "textInputAfter"
                Keys.priority: Keys.AfterItem
                Keys.onLeftPressed: (event) => {
                    root.textInputLeftFired = true; // should NOT fire since C++ TextInput accepts Left
                    event.accepted = true;
                }
            }

            Shortcut {
                sequence: Qt.Key_Q
                enabled: true
                onActivated: {
                    root.shortcutFired = true;
                }
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    auto *terminal = qobject_cast<Terminal*>(object.get());
    QVERIFY(terminal);
    TerminalEngine engine(terminal);

    // 1. Test specific key signals
    engine.handleKeyInput(Qt::Key_Return, QStringLiteral("\n"), 0);
    QCOMPARE(terminal->property("returnPressedFired").toBool(), true);

    engine.handleKeyInput(Qt::Key_Escape, {}, 0);
    QCOMPARE(terminal->property("escapePressedFired").toBool(), true);

    engine.handleKeyInput(Qt::Key_Tab, {}, 0);
    QCOMPARE(terminal->property("tabPressedFired").toBool(), true);

    // 2. Test generic onPressed
    engine.handleKeyInput(Qt::Key_A, QStringLiteral("A"), 0);
    QCOMPARE(terminal->property("genericPressedFired").toBool(), true);
    QCOMPARE(terminal->property("lastGenericKey").toInt(), (int)Qt::Key_A);

    // 3. Test forwardTo
    engine.handleKeyInput(Qt::Key_B, QStringLiteral("B"), 0);
    QCOMPARE(terminal->property("childPressedFired").toBool(), true);

    // 4. Test priority: BeforeItem
    auto* textInputBefore = terminal->findChild<Item*>(QStringLiteral("textInputBefore"));
    QVERIFY(textInputBefore);
    textInputBefore->setFocus(true);
    engine.handleKeyInput(Qt::Key_Left, {}, 0);
    QCOMPARE(terminal->property("textInputLeftFired").toBool(), true);

    // Reset and test AfterItem
    terminal->setProperty("textInputLeftFired", false);
    auto* textInputAfter = terminal->findChild<Item*>(QStringLiteral("textInputAfter"));
    QVERIFY(textInputAfter);
    textInputAfter->setFocus(true);
    engine.handleKeyInput(Qt::Key_Left, {}, 0);
    QCOMPARE(terminal->property("textInputLeftFired").toBool(), false); // C++ consumed it first

    // 5. Test Shortcut
    engine.handleKeyInput(Qt::Key_Q, {}, 0);
    QCOMPARE(terminal->property("shortcutFired").toBool(), true);
}

void TestTerminalFramework::alternateScreenDefersMessages()
{
    ScopedMessageCapture capture;
    const QString deferred = QStringLiteral("qamui-deferred-message");
    const QString afterStop = QStringLiteral("qamui-message-after-stop");
    const QString afterDestroy = QStringLiteral("qamui-message-after-destroy");

    {
        QStringList presented;
        auto routing = QtMessageRouting::install(
            [](const QtMessage&) {},
            [&](const QtMessage& msg) { presented.append(msg.message); }
        );

        Terminal terminal;
        terminal.engine()->setTestViewportSize(QSize(10, 3));
        terminal.engine()->renderFrame();

        qInfo().noquote() << deferred;
        QVERIFY(!presented.contains(deferred));

        terminal.engine()->stop();
        QVERIFY(presented.contains(deferred));

        qInfo().noquote() << afterStop;
        QVERIFY(presented.contains(afterStop));
    }

    qInfo().noquote() << afterDestroy;
    QVERIFY(capture.messages.contains(afterDestroy));
}

void TestTerminalFramework::nestedAlternateScreensDelayFlush()
{
    const QString deferred = QStringLiteral("qamui-nested-deferred-message");
    QStringList presented;
    auto routing = QtMessageRouting::install(
        [](const QtMessage&) {},
        [&](const QtMessage& msg) { presented.append(msg.message); }
    );

    Terminal first;
    Terminal second;
    first.engine()->setTestViewportSize(QSize(10, 3));
    second.engine()->setTestViewportSize(QSize(10, 3));
    first.engine()->renderFrame();
    second.engine()->renderFrame();

    qInfo().noquote() << deferred;
    QVERIFY(!presented.contains(deferred));

    first.engine()->stop();
    QVERIFY(!presented.contains(deferred));

    second.engine()->stop();
    QVERIFY(presented.contains(deferred));
}

void TestTerminalFramework::normalScreenForwardsMessagesImmediately()
{
    const QString immediate = QStringLiteral("qamui-immediate-message");
    QStringList presented;
    auto routing = QtMessageRouting::install(
        [](const QtMessage&) {},
        [&](const QtMessage& msg) { presented.append(msg.message); }
    );

    Terminal terminal;
    terminal.setMode(Terminal::Mode::Inline);
    terminal.engine()->setTestViewportSize(QSize(10, 3));
    terminal.engine()->renderFrame();

    qInfo().noquote() << immediate;
    QVERIFY(presented.contains(immediate));
}

void TestTerminalFramework::terminalTitle()
{
    Terminal terminal;
    QCOMPARE(terminal.title(), QString());

    QSignalSpy spy(&terminal, &Terminal::titleChanged);
    terminal.setTitle(QStringLiteral("My Terminal App"));
    QCOMPARE(terminal.title(), QStringLiteral("My Terminal App"));
    QCOMPARE(spy.count(), 1);

    // Setting same title does not re-emit
    terminal.setTitle(QStringLiteral("My Terminal App"));
    QCOMPARE(spy.count(), 1);

    // Test engine title reflects and helper formats sequence
    QCOMPARE(terminal.engine()->title(), QStringLiteral("My Terminal App"));
    QCOMPARE(TerminalEngine::titleSequence(QStringLiteral("Hello")), std::string("\033]0;Hello\a"));

    // Test QML loading with title binding
    QQmlEngine qmlEngine;
    QQmlComponent component(&qmlEngine);
    component.setData(R"(
        import QtQuick
        import Qamui

        Terminal {
            property string appName: "Qamui"
            title: appName + " - Dashboard"
        }
    )", QUrl());

    auto* root = qobject_cast<Terminal*>(component.create());
    QVERIFY(root);
    QCOMPARE(root->title(), QStringLiteral("Qamui - Dashboard"));

    root->setProperty("appName", QStringLiteral("SuperApp"));
    QCOMPARE(root->title(), QStringLiteral("SuperApp - Dashboard"));
    delete root;
}

void TestTerminalFramework::terminalBackgroundColor()
{
    // 1. Test helper formats sequence
    QCOMPARE(TerminalEngine::osc11Sequence(Color("#1e1e2e")), std::string("\033]11;#1e1e2e\a"));
    QCOMPARE(TerminalEngine::osc11Sequence(Color("blue")), std::string("\033]11;#0000ff\a"));
    QCOMPARE(TerminalEngine::osc11Sequence(Color("transparent")), std::string());
    QCOMPARE(TerminalEngine::osc11Sequence(Color()), std::string());
    QCOMPARE(TerminalEngine::osc111Sequence(), std::string("\033]111\a\033]111;\a\033]111;\033\\"));

    // Test OSC 11 reply color parsing (X11 rgb:rrrr/gggg/bbbb, rgb:rr/gg/bb, #rrggbb)
    QCOMPARE(TerminalEngine::parseOsc11Color("11;rgb:1313/1313/1313"), Color("#131313"));
    QCOMPARE(TerminalEngine::parseOsc11Color("11;rgb:2828/2a2a/3636"), Color("#282a36"));
    QCOMPARE(TerminalEngine::parseOsc11Color("11;rgb:ff/00/00"), Color("#ff0000"));
    QCOMPARE(TerminalEngine::parseOsc11Color("11;#abcdef"), Color("#abcdef"));
    QCOMPARE(TerminalEngine::parseOsc11Color("11;invalid"), Color());
    QCOMPARE(TerminalEngine::parseOsc11Color("10;rgb:1212/1212/1212"), Color());

    // 2. Test Terminal engine tracking with enabled terminal output
    Terminal terminal;
    auto* engine = terminal.engine();
    engine->setTerminalOutputEnabled(true);

    QCOMPARE(engine->isBackgroundOsc11Active(), false);
    QCOMPARE(engine->appliedBackgroundColor(), Color());
    QCOMPARE(engine->originalBackgroundColor(), Color());

    // Feeding an OSC 11 response records the original terminal background color
    engine->feedInput("\033]11;rgb:1313/1313/1313\033\\");
    QCOMPARE(engine->originalBackgroundColor(), Color("#131313"));

    // Setting an opaque color in FullScreen mode activates OSC 11 tracking
    terminal.setColor(Color("#282a36"));
    QCOMPARE(engine->isBackgroundOsc11Active(), true);
    QCOMPARE(engine->appliedBackgroundColor(), Color("#282a36"));

    // Setting same color does not change state
    terminal.setColor(Color("#282a36"));
    QCOMPARE(engine->isBackgroundOsc11Active(), true);

    // Setting transparent color resets tracking
    terminal.setColor(Color("transparent"));
    QCOMPARE(engine->isBackgroundOsc11Active(), false);
    QCOMPARE(engine->appliedBackgroundColor(), Color());

    // Setting opaque color again
    terminal.setColor(Color("#112233"));
    QCOMPARE(engine->isBackgroundOsc11Active(), true);
    QCOMPARE(engine->appliedBackgroundColor(), Color("#112233"));

    // Switching to Inline mode deactivates OSC 11 tracking
    terminal.setMode(Terminal::Mode::Inline);
    QCOMPARE(engine->isBackgroundOsc11Active(), false);
    QCOMPARE(engine->appliedBackgroundColor(), Color());

    // Switching back to FullScreen mode reactivates OSC 11 tracking
    terminal.setMode(Terminal::Mode::FullScreen);
    QCOMPARE(engine->isBackgroundOsc11Active(), true);
    QCOMPARE(engine->appliedBackgroundColor(), Color("#112233"));

    // Stopping the engine resets tracking
    engine->stop();
    QCOMPARE(engine->isBackgroundOsc11Active(), false);

    // 3. Test QML loading with color binding
    QQmlEngine qmlEngine;
    QQmlComponent component(&qmlEngine);
    component.setData(R"(
        import QtQuick
        import Qamui

        Terminal {
            id: root
            property string themeColor: "#282a36"
            color: themeColor
        }
    )", QUrl());

    auto* root = qobject_cast<Terminal*>(component.create());
    QVERIFY(root);
    QCOMPARE(root->color(), Color("#282a36"));

    root->setProperty("themeColor", QStringLiteral("#ff5555"));
    QCOMPARE(root->color(), Color("#ff5555"));
    delete root;
}

QTEST_MAIN(TestTerminalFramework)
#include "test_terminal_framework.moc"
