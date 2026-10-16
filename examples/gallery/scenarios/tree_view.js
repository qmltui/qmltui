/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("TreeView · Project Explorer");
    terminal.expectText("src/QmlTui/Core");
    terminal.expectText("src/QmlTui/Controls");
    terminal.expectText("examples/gallery");
    terminal.expectText("docs/qml-types");
    terminal.expectText("currentIndex: 0");
    terminal.expectText("depth: 0");
    terminal.expectText("expanded: true");
    terminal.marker("Initial tree state");
    terminal.sleep(400);

    // Move to src/QmlTui/Core and toggle collapse with Space.
    terminal.press("Down");
    terminal.waitForText("currentIndex: 1");
    terminal.expectText("depth: 1");
    terminal.expectText("Node: src/QmlTui/Core");
    terminal.sleep(300);

    terminal.press("Space");
    terminal.waitForText("expanded: false");
    terminal.expectText("Last: Core");
    terminal.marker("Collapsed node");
    terminal.sleep(400);

    // Expand all nodes with E, then switch branch icon preset with I.
    terminal.typeText("e");
    terminal.waitForText("Panel.qml");
    terminal.expectText("treeview.md");
    terminal.marker("Expanded all");
    terminal.sleep(400);

    terminal.typeText("i");
    terminal.waitForText("Preset: [+] / [-]");
    terminal.sleep(300);

    // Collapse all with C, then expand all again.
    terminal.typeText("c");
    terminal.waitForText("expanded: false");
    terminal.sleep(300);

    terminal.typeText("e");
    terminal.waitForText("expanded: true");
    terminal.sleep(300);
}
