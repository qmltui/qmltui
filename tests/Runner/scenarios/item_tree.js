/** @param {Runner} app */
function main(app) {
    const visibleTree = app.itemTree();
    if (!visibleTree || visibleTree.width !== 80 || visibleTree.height !== 24)
        app.fail("Expected 80x24 root node, got " + JSON.stringify(visibleTree));

    function findNode(node, objectName) {
        if (!node)
            return null;
        if (node.objectName === objectName)
            return node;
        const children = node.children || [];
        for (let i = 0; i < children.length; ++i) {
            const match = findNode(children[i], objectName);
            if (match)
                return match;
        }
        return null;
    }

    const status = findNode(visibleTree, "statusText");
    if (!status || status.type !== "Text" || status.x !== 1 || status.y !== 3)
        app.fail("Unexpected statusText node: " + JSON.stringify(status));
    if (findNode(visibleTree, "hiddenMarker") !== null)
        app.fail("hiddenMarker must be omitted unless includeHidden is true");

    const fullTree = app.itemTree({ includeHidden: true });
    const hidden = findNode(fullTree, "hiddenMarker");
    if (!hidden || hidden.visible !== false)
        app.fail("Expected hiddenMarker with visible=false, got " + JSON.stringify(hidden));

    // Clicking the TextInput moves activeFocus onto it.
    app.click(5, 5);
    const afterClick = app.itemTree();
    const input = (afterClick.children || []).find(function (child) {
        return child.type === "TextInput";
    });
    if (!input || !input.activeFocus)
        app.fail("Expected TextInput to hold activeFocus: " + JSON.stringify(input));
}
