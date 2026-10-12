#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QSignalSpy>
#include <memory>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/Transition.h>
#include <QmlTui/Core/Engine/Animation.h>
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
#include <QmlTui/Core/Engine/StateGroup.h>
#endif
#include "../TestPaint.h"

using namespace qmltui;

static void addQmlTuiImportPath(QQmlEngine &engine) {
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../QmlTui"));
}

class TestStates : public QObject {
    Q_OBJECT

private slots:
    // Legacy maps & snapshot restoration (runs in both minimal and advanced builds)
    void testLegacyMapProperties();
    void testLegacyMapActiveKeyRemoval();
    void testLegacyMapTargetChange();

    // Advanced features
    void testDirectSyntaxAndGroupedPaths();
    void testBindingLifecycleAndRestoration();
    void testAToBToBaseChainedRestoration();
    void testExplicitTrueVsFalse();
    void testRestoreEntryValuesFalse();
    void testIndependentStateGroups();
    void testAtomicOverlapRejection();
    void testAliasCollisionRejection();
    void testReentrantStateRequests();
    void testObjectDestruction();
    void testZeroDurationTransition();
    void testInterruptedTransitionFromDisplayedValue();
    void testDuplicateWriteSuppression();
    void testBehaviorInteraction();
    void testInvalidDeclarationsRejection();
    void testDisabledFeaturesAvailability();
};

void TestStates::testLegacyMapProperties() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            property int count: 10
            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        target: root
                        properties: ({
                            "count": 42
                        })
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    QCOMPARE(root->property("count").toInt(), 10);

    root->setState("active");
    QCOMPARE(root->state(), QStringLiteral("active"));
    QCOMPARE(root->property("count").toInt(), 42);

    root->setState("");
    QCOMPARE(root->state(), QStringLiteral(""));
    QCOMPARE(root->property("count").toInt(), 10);
}

void TestStates::testLegacyMapActiveKeyRemoval() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            property int a: 1
            property int b: 2
            property var dynamicMap: ({ "a": 10, "b": 20 })
            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        id: pc
                        target: root
                        properties: root.dynamicMap
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    root->setState("active");
    QCOMPARE(root->property("a").toInt(), 10);
    QCOMPARE(root->property("b").toInt(), 20);

    // Update map removing 'b' and modifying 'a'
    QVariantMap newMap;
    newMap.insert("a", 100);
    root->setProperty("dynamicMap", newMap);

    QCOMPARE(root->property("a").toInt(), 100);
    // 'b' was removed from the active map: restored to base value 2
    QCOMPARE(root->property("b").toInt(), 2);

    root->setState("");
    QCOMPARE(root->property("a").toInt(), 1);
    QCOMPARE(root->property("b").toInt(), 2);
}

void TestStates::testLegacyMapTargetChange() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            property Item targetItem: item1
            Item {
                id: item1
                property int val: 5
            }
            Item {
                id: item2
                property int val: 10
            }
            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        id: pc
                        target: root.targetItem
                        properties: ({ "val": 99 })
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *item1 = root->findChild<Item *>(QStringLiteral("item1"));
    auto *item2 = root->findChild<Item *>(QStringLiteral("item2"));
    if (!item1 || !item2) {
        // Find children by iteration
        const auto children = root->findChildren<Item *>();
        if (children.size() >= 2) {
            item1 = children.at(0);
            item2 = children.at(1);
        }
    }
    QVERIFY(item1);
    QVERIFY(item2);

    root->setState("active");
    QCOMPARE(item1->property("val").toInt(), 99);
    QCOMPARE(item2->property("val").toInt(), 10);

    // Retarget to item2
    root->setProperty("targetItem", QVariant::fromValue(item2));
    // item1 restored to base (5), item2 gets 99
    QCOMPARE(item1->property("val").toInt(), 5);
    QCOMPARE(item2->property("val").toInt(), 99);

    root->setState("");
    QCOMPARE(item1->property("val").toInt(), 5);
    QCOMPARE(item2->property("val").toInt(), 10);
}

#if defined(QMLTUI_ENABLE_PRIVATE_QT)

void TestStates::testDirectSyntaxAndGroupedPaths() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property string status: "idle"
            property alias labelAlias: label.text
            Rectangle {
                id: card
                width: 20
                height: 5
                border.color: "white"
                border.style: Border.Single
                Text {
                    id: label
                    text: "initial"
                }
            }
            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        target: card
                        border.color: "blue"
                    }
                    PropertyChanges {
                        label.text: root.status
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *card = root->findChild<Rectangle *>();
    QVERIFY(card);
    auto *label = root->findChild<Text *>();
    QVERIFY(label);

    QCOMPARE(card->border()->color(), QStringLiteral("white"));
    QCOMPARE(label->text(), QStringLiteral("initial"));

    root->setState("active");
    QCOMPARE(card->border()->color(), QStringLiteral("blue"));
    QCOMPARE(label->text(), QStringLiteral("idle"));

    // Verify alias access reflects change
    QCOMPARE(root->property("labelAlias").toString(), QStringLiteral("idle"));

    root->setState("");
    QCOMPARE(card->border()->color(), QStringLiteral("white"));
    QCOMPARE(label->text(), QStringLiteral("initial"));
}

void TestStates::testBindingLifecycleAndRestoration() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property string prefix: "Count"
            property int count: 1
            property string extra: "!"
            Text {
                id: label
                text: root.prefix + ": " + root.count + root.extra
            }
            states: [
                State {
                    name: "overridden"
                    PropertyChanges {
                        target: label
                        text: "Override " + root.count
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *label = root->findChild<Text *>();
    QVERIFY(label);

    QCOMPARE(label->text(), QStringLiteral("Count: 1!"));

    // Enter state: original binding suspended, state expression takes over
    root->setState("overridden");
    QCOMPARE(label->text(), QStringLiteral("Override 1"));

    // Change dependency while active: state expression is reactive (explicit: false default)
    root->setProperty("count", 2);
    QCOMPARE(label->text(), QStringLiteral("Override 2"));

    // Modify a dependency of the suspended original binding
    root->setProperty("prefix", QStringLiteral("Total"));
    root->setProperty("extra", QStringLiteral("?"));
    // Still overridden:
    QCOMPARE(label->text(), QStringLiteral("Override 2"));

    // Exit state: original binding is restored and re-evaluated (never a stale snapshot!)
    root->setState("");
    QCOMPARE(label->text(), QStringLiteral("Total: 2?"));
}

void TestStates::testAToBToBaseChainedRestoration() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int baseOffset: 10
            property int computedVal: root.baseOffset * 2
            Item {
                id: targetItem
                property int value: root.computedVal
            }
            states: [
                State {
                    name: "stateA"
                    PropertyChanges {
                        target: targetItem
                        value: 100
                    }
                },
                State {
                    name: "stateB"
                    PropertyChanges {
                        target: targetItem
                        value: 200
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *target = root->findChild<Item *>();
    QVERIFY(target);
    QCOMPARE(target->property("value").toInt(), 20);

    // Base -> State A
    root->setState("stateA");
    QCOMPARE(target->property("value").toInt(), 100);

    // State A -> State B
    root->setState("stateB");
    QCOMPARE(target->property("value").toInt(), 200);

    // Change underlying dependency while in State B
    root->setProperty("baseOffset", 15);
    QCOMPARE(target->property("value").toInt(), 200);

    // State B -> Base (original base binding must survive across chained transitions!)
    root->setState("");
    QCOMPARE(target->property("value").toInt(), 30);
}

void TestStates::testExplicitTrueVsFalse() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property string msg: "initial"
            Text {
                id: labelReactive
                text: "baseR"
            }
            Text {
                id: labelExplicit
                text: "baseE"
            }
            states: [
                State {
                    name: "testState"
                    PropertyChanges {
                        target: labelReactive
                        explicit: false
                        text: root.msg
                    }
                    PropertyChanges {
                        target: labelExplicit
                        explicit: true
                        text: root.msg
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    const auto texts = root->findChildren<Text *>();
    QCOMPARE(texts.size(), 2);
    Text *labelReactive = texts.at(0);
    Text *labelExplicit = texts.at(1);

    root->setState("testState");
    QCOMPARE(labelReactive->text(), QStringLiteral("initial"));
    QCOMPARE(labelExplicit->text(), QStringLiteral("initial"));

    // Modify dependency
    root->setProperty("msg", QStringLiteral("updated"));
    // explicit: false updates reactively
    QCOMPARE(labelReactive->text(), QStringLiteral("updated"));
    // explicit: true remains frozen until reentry
    QCOMPARE(labelExplicit->text(), QStringLiteral("initial"));

    // Reenter state: explicit re-evaluates on entry
    root->setState("");
    root->setState("testState");
    QCOMPARE(labelReactive->text(), QStringLiteral("updated"));
    QCOMPARE(labelExplicit->text(), QStringLiteral("updated"));
}

void TestStates::testRestoreEntryValuesFalse() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            Text {
                id: label
                text: "baseText"
            }
            states: [
                State {
                    name: "noRestore"
                    PropertyChanges {
                        target: label
                        restoreEntryValues: false
                        text: "finalText"
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *label = root->findChild<Text *>();
    QVERIFY(label);
    QCOMPARE(label->text(), QStringLiteral("baseText"));

    root->setState("noRestore");
    QCOMPARE(label->text(), QStringLiteral("finalText"));

    // Exit state: restoreEntryValues: false retains final value and discards saved predecessor
    root->setState("");
    QCOMPARE(label->text(), QStringLiteral("finalText"));
}

void TestStates::testIndependentStateGroups() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int focusStateVal: 0
            property string connectionStateVal: "offline"

            // Item-level states managing focusStateVal
            states: [
                State {
                    name: "focused"
                    PropertyChanges {
                        target: root
                        focusStateVal: 1
                    }
                }
            ]

            // Independent StateGroup managing connectionStateVal
            StateGroup {
                id: networkGroup
                states: [
                    State {
                        name: "online"
                        PropertyChanges {
                            target: root
                            connectionStateVal: "connected"
                        }
                    }
                ]
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    auto *group = root->findChild<StateGroup *>();
    QVERIFY(group);

    QCOMPARE(root->property("focusStateVal").toInt(), 0);
    QCOMPARE(root->property("connectionStateVal").toString(), QStringLiteral("offline"));

    // Activate Item state
    root->setState("focused");
    QCOMPARE(root->property("focusStateVal").toInt(), 1);
    QCOMPARE(root->property("connectionStateVal").toString(), QStringLiteral("offline"));

    // Activate StateGroup state independently
    group->setState("online");
    QCOMPARE(root->property("focusStateVal").toInt(), 1);
    QCOMPARE(root->property("connectionStateVal").toString(), QStringLiteral("connected"));

    // Deactivate Item state without affecting StateGroup
    root->setState("");
    QCOMPARE(root->property("focusStateVal").toInt(), 0);
    QCOMPARE(root->property("connectionStateVal").toString(), QStringLiteral("connected"));

    // Deactivate StateGroup
    group->setState("");
    QCOMPARE(root->property("focusStateVal").toInt(), 0);
    QCOMPARE(root->property("connectionStateVal").toString(), QStringLiteral("offline"));
}

void TestStates::testAtomicOverlapRejection() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int sharedProp: 0
            property int safeProp: 10

            StateGroup {
                id: groupA
                states: [
                    State {
                        name: "owner"
                        PropertyChanges {
                            target: root
                            sharedProp: 100
                        }
                    }
                ]
            }

            StateGroup {
                id: groupB
                states: [
                    State {
                        name: "colliding"
                        PropertyChanges {
                            target: root
                            sharedProp: 200
                            safeProp: 999
                        }
                    }
                ]
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    const auto groups = root->findChildren<StateGroup *>();
    QCOMPARE(groups.size(), 2);
    StateGroup *groupA = groups.at(0);
    StateGroup *groupB = groups.at(1);

    // Group A claims ownership of sharedProp
    groupA->setState("owner");
    QCOMPARE(groupA->state(), QStringLiteral("owner"));
    QCOMPARE(root->property("sharedProp").toInt(), 100);

    // Group B attempts to enter state that mutates sharedProp:
    // Preflight must reject the entire activation atomically!
    // safeProp must NOT be modified either!
    groupB->setState("colliding");
    // State remains rejected / intact
    QCOMPARE(groupB->state(), QStringLiteral(""));
    QCOMPARE(root->property("sharedProp").toInt(), 100);
    QCOMPARE(root->property("safeProp").toInt(), 10);

    // Once Group A releases ownership:
    groupA->setState("");
    QCOMPARE(root->property("sharedProp").toInt(), 0);

    // Group B can now activate cleanly
    groupB->setState("colliding");
    QCOMPARE(groupB->state(), QStringLiteral("colliding"));
    QCOMPARE(root->property("sharedProp").toInt(), 200);
    QCOMPARE(root->property("safeProp").toInt(), 999);
}

void TestStates::testAliasCollisionRejection() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property alias textAlias: targetLabel.text
            Text {
                id: targetLabel
                text: "base"
            }

            StateGroup {
                id: groupA
                states: [
                    State {
                        name: "viaAlias"
                        PropertyChanges {
                            target: root
                            textAlias: "ownedByAlias"
                        }
                    }
                ]
            }

            StateGroup {
                id: groupB
                states: [
                    State {
                        name: "viaDirect"
                        PropertyChanges {
                            target: targetLabel
                            text: "collision"
                        }
                    }
                ]
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    const auto groups = root->findChildren<StateGroup *>();
    QCOMPARE(groups.size(), 2);
    StateGroup *groupA = groups.at(0);
    StateGroup *groupB = groups.at(1);
    auto *label = root->findChild<Text *>();
    QVERIFY(label);

    groupA->setState("viaAlias");
    QCOMPARE(label->text(), QStringLiteral("ownedByAlias"));

    // Group B tries to mutate targetLabel.text directly - resolves to same canonical property
    groupB->setState("viaDirect");
    // Rejected!
    QCOMPARE(groupB->state(), QStringLiteral(""));
    QCOMPARE(label->text(), QStringLiteral("ownedByAlias"));
}

void TestStates::testReentrantStateRequests() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int step: 0
            onStateChanged: {
                if (state === "A") {
                    // Reentrant request while in state transition
                    state = "B"
                }
            }
            states: [
                State {
                    name: "A"
                    PropertyChanges {
                        target: root
                        step: 1
                    }
                },
                State {
                    name: "B"
                    PropertyChanges {
                        target: root
                        step: 2
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    root->setState("A");
    // Latest pending reentrant request (B) must be honored
    QCOMPARE(root->state(), QStringLiteral("B"));
    QCOMPARE(root->property("step").toInt(), 2);
}

void TestStates::testObjectDestruction() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            Item {
                id: dynamicTarget
                property int count: 10
            }
            StateGroup {
                id: group
                states: [
                    State {
                        name: "active"
                        PropertyChanges {
                            target: dynamicTarget
                            properties: ({ "count": 50 })
                        }
                    }
                ]
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    auto *root = component.create();
    QVERIFY(root);

    auto *group = root->findChild<StateGroup *>();
    QVERIFY(group);
    auto *dynamicTarget = root->findChild<Item *>();
    QVERIFY(dynamicTarget);

    group->setState("active");
    QCOMPARE(dynamicTarget->property("count").toInt(), 50);

    // Destroy target while active: must cancel affected work safely
    delete dynamicTarget;

    // StateGroup deactivates / changes state without crash
    group->setState("");

    delete root;
}

void TestStates::testZeroDurationTransition() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int value: 0
            states: [
                State {
                    name: "fast"
                    PropertyChanges {
                        target: root
                        value: 100
                    }
                }
            ]
            transitions: [
                Transition {
                    NumberAnimation {
                        duration: 0
                        loops: -1
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    // Apply zero-duration transition: completes synchronously without busy loop
    root->setState("fast");
    QCOMPARE(root->property("value").toInt(), 100);
    QCOMPARE(root->state(), QStringLiteral("fast"));
}

void TestStates::testInterruptedTransitionFromDisplayedValue() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            width: 200
            states: [
                State {
                    name: "A"
                    PropertyChanges {
                        target: root
                        width: 100
                    }
                },
                State {
                    name: "B"
                    PropertyChanges {
                        target: root
                        width: 0
                    }
                }
            ]
            transitions: [
                Transition {
                    NumberAnimation {
                        duration: 100
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    QCOMPARE(root->width(), 200);

    // Start transition to A (200 -> 100)
    root->setState("A");

    auto *anim = root->findChild<PropertyAnimation *>();
    QVERIFY(anim);
    QVERIFY(anim->running());

    // Advance 50ms (halfway: 200 -> 150)
    anim->tick(0);
    anim->tick(50);
    int displayedMidpoint = root->width();
    QCOMPARE(displayedMidpoint, 150);

    // Interrupt with state B: replacement transition must start from currently displayed value (150)
    root->setState("B");
    auto *replacementAnim = root->findChild<PropertyAnimation *>();
    QVERIFY(replacementAnim);
    QCOMPARE(replacementAnim->from().toInt(), 150);
    QCOMPARE(replacementAnim->to().toInt(), 0);
}

void TestStates::testDuplicateWriteSuppression() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int cellX: 10
            states: [
                State {
                    name: "same"
                    PropertyChanges {
                        target: root
                        cellX: 10
                    }
                }
            ]
            transitions: [
                Transition {
                    NumberAnimation {
                        duration: 100
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    // When endpoint equals current value, no animation is allocated
    root->setState("same");
    auto *anim = root->findChild<PropertyAnimation *>();
    QVERIFY(!anim || !anim->running());
    QCOMPARE(root->property("cellX").toInt(), 10);
}

void TestStates::testBehaviorInteraction() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int animatedWithTransition: 0
            property int animatedWithBehavior: 0

            Behavior on animatedWithTransition {
                NumberAnimation { duration: 500 }
            }
            Behavior on animatedWithBehavior {
                NumberAnimation { duration: 200 }
            }

            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        target: root
                        animatedWithTransition: 100
                        animatedWithBehavior: 50
                    }
                }
            ]

            // Matching transition only for animatedWithTransition
            transitions: [
                Transition {
                    NumberAnimation {
                        property: "animatedWithTransition"
                        duration: 100
                    }
                }
            ]
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *root = qobject_cast<Item *>(object.get());
    QVERIFY(root);

    root->setState("active");
    // Both animations run: transition for animatedWithTransition, Behavior for animatedWithBehavior
    const auto anims = root->findChildren<PropertyAnimation *>();
    QVERIFY(anims.size() >= 2);
}

void TestStates::testInvalidDeclarationsRejection() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    // 1. Mixing map entries and direct assignments is rejected
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            Item {
                id: root
                property int a: 1
                states: [
                    State {
                        name: "invalid"
                        PropertyChanges {
                            target: root
                            a: 10
                            properties: ({ "b": 20 })
                        }
                    }
                ]
            }
        )", QUrl());
        QVERIFY(!component.isReady());
        QVERIFY(component.errorString().contains("mix properties map and direct assignments"));
    }

    // 2. Creating state-specific objects is rejected
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            Item {
                id: root
                states: [
                    State {
                        name: "invalidObj"
                        PropertyChanges {
                            target: root
                            Item { }
                        }
                    }
                ]
            }
        )", QUrl());
        QVERIFY(!component.isReady());
        QVERIFY(component.errorString().contains("creating state-specific objects"));
    }
}

#else

void TestStates::testDirectSyntaxAndGroupedPaths() { QSKIP("Advanced states disabled"); }
void TestStates::testBindingLifecycleAndRestoration() { QSKIP("Advanced states disabled"); }
void TestStates::testAToBToBaseChainedRestoration() { QSKIP("Advanced states disabled"); }
void TestStates::testExplicitTrueVsFalse() { QSKIP("Advanced states disabled"); }
void TestStates::testRestoreEntryValuesFalse() { QSKIP("Advanced states disabled"); }
void TestStates::testIndependentStateGroups() { QSKIP("Advanced states disabled"); }
void TestStates::testAtomicOverlapRejection() { QSKIP("Advanced states disabled"); }
void TestStates::testAliasCollisionRejection() { QSKIP("Advanced states disabled"); }
void TestStates::testReentrantStateRequests() { QSKIP("Advanced states disabled"); }
void TestStates::testObjectDestruction() { QSKIP("Advanced states disabled"); }
void TestStates::testZeroDurationTransition() { QSKIP("Advanced states disabled"); }
void TestStates::testInterruptedTransitionFromDisplayedValue() { QSKIP("Advanced states disabled"); }
void TestStates::testDuplicateWriteSuppression() { QSKIP("Advanced states disabled"); }
void TestStates::testBehaviorInteraction() { QSKIP("Advanced states disabled"); }
void TestStates::testInvalidDeclarationsRejection() { QSKIP("Advanced states disabled"); }

#endif

void TestStates::testDisabledFeaturesAvailability() {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QSKIP("Private Qt features are enabled in this build");
#else
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    // When advanced states are disabled, StateGroup is not available in QML
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            StateGroup { }
        }
    )", QUrl());
    QVERIFY(!component.isReady());
#endif
}


QTEST_GUILESS_MAIN(TestStates)
#include "test_states.moc"
