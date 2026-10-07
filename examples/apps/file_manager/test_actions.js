// test_actions.js - Tests marking/selection, help modal, dotfiles toggle, and sorting

function main(terminal) {
    terminal.waitForText("NORMAL", 2000);

    // 1. Selection / Marking
    terminal.press("Space");
    terminal.waitForText("[1 marked]", 2000);
    terminal.expectText("[✓]");

    // Clear mark
    terminal.press("Space");
    terminal.expectNoText("[1 marked]");

    // 2. Help Modal Dialog
    terminal.press("?");
    terminal.waitForText("KEYBINDINGS", 2000);
    terminal.expectText("NAVIGATION");
    terminal.expectText("SELECTION & ACTIONS");

    // Close Help Modal
    terminal.press("Escape");
    terminal.waitForText("NORMAL", 2000);
    terminal.expectNoText("KEYBINDINGS");

    // 3. Toggle Dotfiles
    terminal.press(".");
    terminal.waitForText(".git", 2000);
    terminal.expectText(".env");

    // 4. Cycle Sort Mode
    terminal.press("s");
    terminal.waitForText("Sort: size", 2000);

    terminal.captureText("actions.txt");
}
