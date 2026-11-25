#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QSignalSpy>
#include <memory>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Layout.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/StateGroup.h>
#include <QmlTui/Core/Engine/Transition.h>
#include <QmlTui/Core/Engine/Animation.h>
#include "../TestPaint.h"

using namespace qmltui;

static void addQmlTuiImportPath(QQmlEngine& engine) {
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../QmlTui"));
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
    void testNonBasicPropertyChangesAndTargetEviction();
    void testZeroDurationTransition();
    void testInterruptedTransitionFromDisplayedValue();
    void testDuplicateWriteSuppression();
    void testBehaviorInteraction();
    void testInvalidDeclarationsRejection();
    void testTransitionTargetDestroyedMidTransition();
    void testTransitionStateBindingDetachedBeforeLiteralState();
    void testDecodedBindingTeardownWithoutEngine();
    void testColorTransitionStringLiteralsAndZeroDelta();
    void testReverseTransitionToBaseAndInterruption();
    void testReverseColorEndpointsAndMidRevertOwnership();
    void testCustomBindingsDynamicRetargetAndAttachedOmittedTarget();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* item1 = root->findChild<Item*>(QStringLiteral("item1"));
    auto* item2 = root->findChild<Item*>(QStringLiteral("item2"));
    if (!item1 || !item2) {
        // Find children by iteration
        const auto children = root->findChildren<Item*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    auto* root = component.create();
    QVERIFY(root);

    auto* group = root->findChild<StateGroup*>();
    QVERIFY(group);
    auto* dynamicTarget = root->findChild<Item*>();
    QVERIFY(dynamicTarget);

    group->setState("active");
    QCOMPARE(dynamicTarget->property("count").toInt(), 50);

    // Destroy target while active: must cancel affected work safely
    delete dynamicTarget;

    // StateGroup deactivates / changes state without crash
    group->setState("");

    delete root;
}

void TestStates::testNonBasicPropertyChangesAndTargetEviction() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            Item {
                id: t1
                objectName: "t1"
                property real ratio: 0.25
                property size dims: Qt.size(2, 3)
            }
            Text {
                id: t2
                objectName: "t2"
                wrapMode: Text.NoWrap
            }
            Item {
                id: t3
                objectName: "t3"
                property real ratio: 1.5
            }
            states: [
                State {
                    name: "active"
                    PropertyChanges {
                        target: t1
                        properties: ({ "ratio": 0.75, "dims": Qt.size(8, 9) })
                    }
                    PropertyChanges {
                        target: t2
                        properties: ({ "wrapMode": Text.WordWrap })
                    }
                    PropertyChanges {
                        target: t3
                        properties: ({ "ratio": 2.5 })
                    }
                }
            ]
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* t1 = root->findChild<Item*>(QStringLiteral("t1"));
    auto* t2 = root->findChild<Text*>(QStringLiteral("t2"));
    auto* t3 = root->findChild<Item*>(QStringLiteral("t3"));
    QVERIFY(t1 && t2 && t3);

    root->setState(QStringLiteral("active"));
    QCOMPARE(t1->property("ratio").toDouble(), 0.75);
    QCOMPARE(t1->property("dims").toSize(), QSize(8, 9));
    QCOMPARE(t2->wrapMode(), Text::WordWrap);
    QCOMPARE(t3->property("ratio").toDouble(), 2.5);

    // Destroy middle target while multiple CanonicalProperty keys populate std::map/QHash.
    delete t2;

    // Subsequent transitions and controller teardown must remain well-ordered and crash-free.
    root->setState(QString());
    QCOMPARE(t1->property("ratio").toDouble(), 0.25);
    QCOMPARE(t1->property("dims").toSize(), QSize(2, 3));
    QCOMPARE(t3->property("ratio").toDouble(), 1.5);

    root->setState(QStringLiteral("active"));
    QCOMPARE(t1->property("ratio").toDouble(), 0.75);
    QCOMPARE(t3->property("ratio").toDouble(), 2.5);
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* card = root->findChild<Rectangle*>();
    QVERIFY(card);
    auto* label = root->findChild<Text*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* label = root->findChild<Text*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* target = root->findChild<Item*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    const auto texts = root->findChildren<Text*>();
    QCOMPARE(texts.size(), 2);
    Text* labelReactive = texts.at(0);
    Text* labelExplicit = texts.at(1);

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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* label = root->findChild<Text*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    auto* group = root->findChild<StateGroup*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    const auto groups = root->findChildren<StateGroup*>();
    QCOMPARE(groups.size(), 2);
    StateGroup* groupA = groups.at(0);
    StateGroup* groupB = groups.at(1);

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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    const auto groups = root->findChildren<StateGroup*>();
    QCOMPARE(groups.size(), 2);
    StateGroup* groupA = groups.at(0);
    StateGroup* groupB = groups.at(1);
    auto* label = root->findChild<Text*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    root->setState("A");
    // Latest pending reentrant request (B) must be honored
    QCOMPARE(root->state(), QStringLiteral("B"));
    QCOMPARE(root->property("step").toInt(), 2);
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    QCOMPARE(root->width(), 200);

    // Start transition to A (200 -> 100)
    root->setState("A");

    auto* anim = root->findChild<PropertyAnimation*>();
    QVERIFY(anim);
    QVERIFY(anim->running());

    // Advance 50ms (halfway: 200 -> 150)
    anim->tick(0);
    anim->tick(50);
    int displayedMidpoint = root->width();
    QCOMPARE(displayedMidpoint, 150);

    // Interrupt with state B: replacement transition must start from currently displayed value
    // (150)
    root->setState("B");
    auto* replacementAnim = root->findChild<PropertyAnimation*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    // When endpoint equals current value, no animation is allocated
    root->setState("same");
    auto* anim = root->findChild<PropertyAnimation*>();
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
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    root->setState("active");
    // Both animations run: transition for animatedWithTransition, Behavior for animatedWithBehavior
    const auto anims = root->findChildren<PropertyAnimation*>();
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
        )",
                          QUrl());
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
        )",
                          QUrl());
        QVERIFY(!component.isReady());
        QVERIFY(component.errorString().contains("creating state-specific objects"));
    }
}

void TestStates::testTransitionStateBindingDetachedBeforeLiteralState() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        Item {
            id: root
            property int baseSource: 10
            property int sourceA: 20
            property int value: root.baseSource
            states: [
                State {
                    name: "A"
                    PropertyChanges {
                        target: root
                        value: root.sourceA * 2
                    }
                },
                State {
                    name: "B"
                    PropertyChanges {
                        target: root
                        value: 100
                    }
                }
            ]
            transitions: [
                Transition {
                    from: "A"
                    to: "B"
                    NumberAnimation {
                        property: "value"
                        duration: 100
                    }
                }
            ]
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);
    QCOMPARE(root->property("value").toInt(), 10);

    root->setState("A");
    QCOMPARE(root->property("value").toInt(), 40);
    root->setProperty("sourceA", 25);
    QCOMPARE(root->property("value").toInt(), 50);

    // Transition A (binding) -> B (literal 100)
    root->setState("B");
    PropertyAnimation* runningAnim = nullptr;
    for (PropertyAnimation* anim : root->findChildren<PropertyAnimation*>()) {
        if (anim->running()) {
            runningAnim = anim;
            break;
        }
    }
    QVERIFY(runningAnim);

    runningAnim->tick(0);
    runningAnim->tick(50);
    QCOMPARE(root->property("value").toInt(), 75);

    // State A's binding must already be detached while animating to State B.
    root->setProperty("sourceA", 99);
    QCOMPARE(root->property("value").toInt(), 75);

    runningAnim->tick(100);
    QCOMPARE(root->property("value").toInt(), 100);

    // State A's binding must not remain active in State B after completion.
    root->setProperty("sourceA", 123);
    QCOMPARE(root->property("value").toInt(), 100);

    // Returning to base state restores the original base binding.
    root->setState("");
    QCOMPARE(root->property("value").toInt(), 10);
    root->setProperty("baseSource", 17);
    QCOMPARE(root->property("value").toInt(), 17);
}

void TestStates::testDecodedBindingTeardownWithoutEngine() {
    std::unique_ptr<Item> root;
    {
        auto engine = std::make_unique<QQmlEngine>();
        addQmlTuiImportPath(*engine);

        QQmlComponent component(engine.get());
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            Item {
                id: root
                property int seed: 7
                property int computed: 1
                states: [
                    State {
                        name: "afterEngine"
                        PropertyChanges {
                            target: root
                            computed: root.seed + 5
                        }
                    }
                ]
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        QObject* created = component.create();
        QVERIFY(created);
        QQmlEngine::setObjectOwnership(created, QQmlEngine::CppOwnership);
        root.reset(qobject_cast<Item*>(created));
        QVERIFY(root);
    }

    // Engine is destroyed; activating a decoded expression binding must not dereference nullptr.
    root->setState("afterEngine");
    QCOMPARE(root->state(), QStringLiteral("afterEngine"));
    root->setState("");
    QCOMPARE(root->state(), QString());
}

void TestStates::testCustomBindingsDynamicRetargetAndAttachedOmittedTarget() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    // 1. Dynamic PropertyChanges.target retargeting with custom bindings
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            Item {
                id: root
                property int baseA: 11
                property int baseB: 22
                property int overrideSeed: 10
                property Item currentTarget: boxA

                Item {
                    id: boxA
                    objectName: "boxA"
                    width: root.baseA
                    height: 5
                }
                Item {
                    id: boxB
                    objectName: "boxB"
                    width: root.baseB
                    height: 6
                }

                states: [
                    State {
                        name: "active"
                        PropertyChanges {
                            target: root.currentTarget
                            width: root.overrideSeed * 3
                            height: 99
                        }
                    }
                ]
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto* root = qobject_cast<Item*>(object.get());
        QVERIFY(root);
        auto* boxA = root->findChild<Item*>(QStringLiteral("boxA"));
        auto* boxB = root->findChild<Item*>(QStringLiteral("boxB"));
        QVERIFY(boxA && boxB);

        root->setState("active");
        QCOMPARE(boxA->width(), 30);
        QCOMPARE(boxA->height(), 99);
        QCOMPARE(boxB->width(), 22);
        QCOMPARE(boxB->height(), 6);

        // Retarget from boxA to boxB while state is active
        root->setProperty("currentTarget", QVariant::fromValue(boxB));
        QCOMPARE(boxA->width(), 11);
        QCOMPARE(boxA->height(), 5);
        root->setProperty("baseA", 14);
        QCOMPARE(boxA->width(), 14);

        QCOMPARE(boxB->width(), 30);
        QCOMPARE(boxB->height(), 99);
        root->setProperty("overrideSeed", 20);
        QCOMPARE(boxB->width(), 60);

        root->setState("");
        QCOMPARE(boxB->width(), 22);
        QCOMPARE(boxB->height(), 6);
        root->setProperty("baseB", 25);
        QCOMPARE(boxB->width(), 25);
    }

    // 2. Attached property assignment in PropertyChanges with omitted target
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            RowLayout {
                id: row
                width: 40
                height: 2
                Item {
                    id: stretchy
                    objectName: "stretchy"
                    implicitWidth: 6
                    height: 1
                    states: [
                        State {
                            name: "expanded"
                            PropertyChanges {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 24
                            }
                        }
                    ]
                }
                Item {
                    implicitWidth: 10
                    height: 1
                }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto* row = qobject_cast<Item*>(object.get());
        QVERIFY(row);
        auto* stretchy = row->findChild<Item*>(QStringLiteral("stretchy"));
        QVERIFY(stretchy);
        TextGridBuffer buf;
        buf.resize(40, 2);
        testing::paintItem(row, buf, 0, 0);
        QCOMPARE(stretchy->width(), 6);

        stretchy->setState("expanded");
        auto* attached =
            qobject_cast<LayoutAttached*>(qmlAttachedPropertiesObject<Layout>(stretchy, false));
        QVERIFY(attached);
        QCOMPARE(attached->fillWidth, true);
        QCOMPARE(attached->preferredWidth, 24);
        testing::paintItem(row, buf, 0, 0);
        QVERIFY(stretchy->width() >= 29);

        stretchy->setState("");
        QCOMPARE(attached->fillWidth, false);
        QCOMPARE(attached->preferredWidth, -1);
        testing::paintItem(row, buf, 0, 0);
        QCOMPARE(stretchy->width(), 6);
    }
}

#else

void TestStates::testDirectSyntaxAndGroupedPaths() {
    QSKIP("Advanced states disabled");
}
void TestStates::testBindingLifecycleAndRestoration() {
    QSKIP("Advanced states disabled");
}
void TestStates::testAToBToBaseChainedRestoration() {
    QSKIP("Advanced states disabled");
}
void TestStates::testExplicitTrueVsFalse() {
    QSKIP("Advanced states disabled");
}
void TestStates::testRestoreEntryValuesFalse() {
    QSKIP("Advanced states disabled");
}
void TestStates::testIndependentStateGroups() {
    QSKIP("Advanced states disabled");
}
void TestStates::testAtomicOverlapRejection() {
    QSKIP("Advanced states disabled");
}
void TestStates::testAliasCollisionRejection() {
    QSKIP("Advanced states disabled");
}
void TestStates::testReentrantStateRequests() {
    QSKIP("Advanced states disabled");
}
void TestStates::testZeroDurationTransition() {
    QSKIP("Advanced states disabled");
}
void TestStates::testInterruptedTransitionFromDisplayedValue() {
    QSKIP("Advanced states disabled");
}
void TestStates::testDuplicateWriteSuppression() {
    QSKIP("Advanced states disabled");
}
void TestStates::testBehaviorInteraction() {
    QSKIP("Advanced states disabled");
}
void TestStates::testInvalidDeclarationsRejection() {
    QSKIP("Advanced states disabled");
}
void TestStates::testTransitionStateBindingDetachedBeforeLiteralState() {
    QSKIP("Advanced states disabled");
}
void TestStates::testDecodedBindingTeardownWithoutEngine() {
    QSKIP("Advanced states disabled");
}
void TestStates::testCustomBindingsDynamicRetargetAndAttachedOmittedTarget() {
    QSKIP("Advanced states disabled");
}

#endif

void TestStates::testTransitionTargetDestroyedMidTransition() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    auto findRunningAnim = [](QObject* scope) -> PropertyAnimation* {
        for (PropertyAnimation* anim : scope->findChildren<PropertyAnimation*>()) {
            if (anim->running()) return anim;
        }
        return nullptr;
    };

    // 1. Destroying a Transition target mid-flight stops and deletes its running animation.
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import QmlTui
            Item {
                id: root
                property Item victim: null
                states: [
                    State {
                        name: "go"
                        PropertyChanges {
                            target: root.victim
                            properties: ({ "width": 100 })
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
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto* root = qobject_cast<Item*>(object.get());
        QVERIFY(root);

        auto victim = std::make_unique<Item>();
        victim->setWidth(10);
        root->setProperty("victim", QVariant::fromValue(victim.get()));

        root->setState("go");
        QPointer<PropertyAnimation> running = findRunningAnim(root);
        QVERIFY(running);
        running->tick(0);
        running->tick(30);
        QCOMPARE(victim->width(), 37);

        victim.reset();
        QVERIFY(!running);
        QVERIFY(!findRunningAnim(root));
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    // 2. Destination binding on a destroyed Transition target must never install onto a dead
    // QObject.
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            pragma ComponentBehavior: Bound
            import QtQml
            import QmlTui
            Item {
                id: root
                property int targetWidth: 80
                property Item victim: null
                states: [
                    State {
                        name: "go"
                        PropertyChanges {
                            target: root.victim
                            width: root.targetWidth
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
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));

        std::unique_ptr<QObject> object(component.create());
        auto* root = qobject_cast<Item*>(object.get());
        QVERIFY(root);

        auto victim = std::make_unique<Item>();
        victim->setWidth(20);
        QQmlProperty staleProp(victim.get(), QStringLiteral("width"));
        QVERIFY(staleProp.isValid());
        root->setProperty("victim", QVariant::fromValue(victim.get()));

        root->setState("go");
        QPointer<PropertyAnimation> running = findRunningAnim(root);
        QVERIFY(running);

        victim.reset();
        QVERIFY(staleProp.isValid());
        QVERIFY(!staleProp.object());
        QVERIFY(!running);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
#endif
}

void TestStates::testColorTransitionStringLiteralsAndZeroDelta() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    auto findRunningAnim = [](QObject* scope) -> PropertyAnimation* {
        for (PropertyAnimation* anim : scope->findChildren<PropertyAnimation*>()) {
            if (anim->running()) return anim;
        }
        return nullptr;
    };

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            property color accent: "cyan"
            states: [
                State {
                    name: "sameHex"
                    PropertyChanges {
                        target: root
                        properties: ({ "accent": "#00ffff" })
                    }
                },
                State {
                    name: "namedTarget"
                    PropertyChanges {
                        target: root
                        properties: ({ "accent": "magenta" })
                    }
                },
                State {
                    name: "ansiTarget"
                    PropertyChanges {
                        target: root
                        properties: ({ "accent": "ansi:red" })
                    }
                }
            ]
            transitions: [
                Transition {
                    ColorAnimation {
                        duration: 100
                    }
                }
            ]
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);

    // "cyan" -> "#00ffff" represents the exact same RGB color: zero-delta suppresses animation.
    root->setState("sameHex");
    QVERIFY(!findRunningAnim(root));
    QCOMPARE(root->property("accent").value<Color>(), Color::fromString(QStringLiteral("#00ffff")));

    // Non-"color"-named property of type `color` with named string literal matches ColorAnimation.
    root->setState("namedTarget");
    PropertyAnimation* anim = findRunningAnim(root);
    QVERIFY(anim);
    QVERIFY(qobject_cast<ColorAnimation*>(anim) != nullptr);
    anim->tick(0);
    anim->tick(50);
    const Color mid = root->property("accent").value<Color>();
    QVERIFY(mid != Color::fromString(QStringLiteral("cyan")));
    QVERIFY(mid != Color::fromString(QStringLiteral("magenta")));
    anim->tick(100);
    QCOMPARE(root->property("accent").value<Color>(), Color::fromString(QStringLiteral("magenta")));

    // ANSI string literal ("ansi:red") also matches ColorAnimation on `property color accent`.
    root->setState("ansiTarget");
    PropertyAnimation* ansiAnim = findRunningAnim(root);
    QVERIFY(ansiAnim);
    QVERIFY(qobject_cast<ColorAnimation*>(ansiAnim) != nullptr);
    ansiAnim->tick(0);
    ansiAnim->tick(100);
    QCOMPARE(root->property("accent").value<Color>(),
             Color::fromString(QStringLiteral("ansi:red")));
}

void TestStates::testReverseTransitionToBaseAndInterruption() {
    auto findRunningAnim = [](QObject* obj) -> PropertyAnimation* {
        for (PropertyAnimation* anim : obj->findChildren<PropertyAnimation*>()) {
            if (anim->running()) return anim;
        }
        return nullptr;
    };

    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            property int baseFactor: 1
            property int sizeVal: baseFactor * 10
            states: [
                State {
                    name: "expanded"
                    PropertyChanges {
                        target: root
                        properties: ({ "sizeVal": 50 })
                    }
                }
            ]
            transitions: [
                Transition {
                    from: "*"
                    to: "*"
                    NumberAnimation {
                        property: "sizeVal"
                        duration: 100
                    }
                }
            ]
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* root = qobject_cast<Item*>(object.get());
    QVERIFY(root);
    QCOMPARE(root->property("sizeVal").toInt(), 10);

    // Enter expanded state and finish transition.
    root->setState("expanded");
    PropertyAnimation* enterAnim = findRunningAnim(root);
    QVERIFY(enterAnim);
    enterAnim->tick(0);
    enterAnim->tick(100);
    QCOMPARE(root->property("sizeVal").toInt(), 50);

    // Exit to base state "": Transition animates smoothly back to 10.
    root->setState("");
    PropertyAnimation* exitAnim = findRunningAnim(root);
    QVERIFY(exitAnim);
    exitAnim->tick(0);
    exitAnim->tick(50);
    const int midValue = root->property("sizeVal").toInt();
    QVERIFY(midValue > 10 && midValue < 50);

    // Interrupt mid-revert by re-entering "expanded": original base value (10) must be preserved.
    root->setState("expanded");
    PropertyAnimation* reenterAnim = findRunningAnim(root);
    QVERIFY(reenterAnim);
    reenterAnim->tick(0);
    reenterAnim->tick(100);
    QCOMPARE(root->property("sizeVal").toInt(), 50);

    // Now exit to base state "" to completion.
    root->setState("");
    PropertyAnimation* finalExitAnim = findRunningAnim(root);
    QVERIFY(finalExitAnim);
    finalExitAnim->tick(0);
    finalExitAnim->tick(100);
    QCOMPARE(root->property("sizeVal").toInt(), 10);

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    // Original base binding (`baseFactor * 10`) is restored on transition completion.
    root->setProperty("baseFactor", 3);
    QCOMPARE(root->property("sizeVal").toInt(), 30);
#endif
}

void TestStates::testReverseColorEndpointsAndMidRevertOwnership() {
    auto findRunningAnim = [](QObject* obj) -> PropertyAnimation* {
        for (PropertyAnimation* anim : obj->findChildren<PropertyAnimation*>()) {
            if (anim->running()) return anim;
        }
        return nullptr;
    };

    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import QmlTui
        Item {
            id: root
            Rectangle {
                id: box
                objectName: "box"
                width: 10
                height: 3
                states: [
                    State {
                        name: "tinted"
                        PropertyChanges {
                            target: box
                            properties: ({ "color": "ansi:red" })
                        }
                    }
                ]
                transitions: [
                    Transition {
                        from: "*"
                        to: "*"
                        ColorAnimation { duration: 100 }
                    }
                ]
            }
            StateGroup {
                id: rival
                objectName: "rival"
                states: [
                    State {
                        name: "blue"
                        PropertyChanges {
                            target: box
                            properties: ({ "color": "#0000ff" })
                        }
                    }
                ]
            }
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* box = object->findChild<Rectangle*>(QStringLiteral("box"));
    auto* rival = object->findChild<StateGroup*>(QStringLiteral("rival"));
    QVERIFY(box);
    QVERIFY(rival);
    QVERIFY(box->color().isTransparent());

    // Enter tinted state: ColorAnimation commits exact ANSI palette tag on finish.
    box->setState(QStringLiteral("tinted"));
    PropertyAnimation* enterAnim = findRunningAnim(box);
    QVERIFY(enterAnim);
    enterAnim->tick(0);
    enterAnim->tick(100);
    QCOMPARE(box->color(), Color::fromString(QStringLiteral("ansi:red")));

    // Exit to base state "": ownership stays held while reverse transition is active.
    box->setState(QString());
    PropertyAnimation* exitAnim = findRunningAnim(box);
    QVERIFY(exitAnim);
    exitAnim->tick(0);
    exitAnim->tick(50);

    // Rival StateGroup must be rejected atomically while reverse transition is ticking.
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression(QStringLiteral("is owned by another StateGroup/Item")));
    rival->setState(QStringLiteral("blue"));
    QCOMPARE(rival->state(), QString());

    // Finish reverse transition: Color::Transparent is restored exactly (not Color::Default).
    exitAnim->tick(50);
    QVERIFY(box->color().isTransparent());

    // Once reverse transition finishes, ownership is released and rival succeeds.
    rival->setState(QStringLiteral("blue"));
    QCOMPARE(rival->state(), QStringLiteral("blue"));
    QCOMPARE(box->color(), Color::fromString(QStringLiteral("#0000ff")));
}

void TestStates::testDisabledFeaturesAvailability() {
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QSKIP("Private Qt features are enabled in this build");
#else
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    // StateGroup is available in public-Qt builds
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import QmlTui
            Item {
                StateGroup { }
            }
        )",
                          QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    }

    // Behavior requires QMLTUI_ENABLE_PRIVATE_QT
    {
        QQmlComponent component(&engine);
        component.setData(R"(
            import QtQml
            import QmlTui
            Item {
                Behavior on width { }
            }
        )",
                          QUrl());
        QVERIFY(!component.isReady());
    }
#endif
}

QTEST_GUILESS_MAIN(TestStates)
#include "test_states.moc"
