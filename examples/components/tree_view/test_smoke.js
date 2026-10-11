/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QT QUICK NATIVE TREEVIEW DEMO");
    terminal.advanceTime(200);
    terminal.captureText("tree_view.txt");
}
