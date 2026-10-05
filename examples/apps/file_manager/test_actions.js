// test_actions.js - Tests marking/selection, help modal, dotfiles toggle, and sorting

function run() {
    Test.waitForText("NORMAL", 2000);

    // 1. Selection / Marking
    Test.press("Space");
    Test.waitForText("[1 marked]", 2000);
    Test.expectText("[✓]");

    // Clear mark
    Test.press("Space");
    Test.expectNoText("[1 marked]");

    // 2. Help Modal Dialog
    Test.press("?");
    Test.waitForText("KEYBINDINGS", 2000);
    Test.expectText("NAVIGATION");
    Test.expectText("SELECTION & ACTIONS");

    // Close Help Modal
    Test.press("Escape");
    Test.waitForText("NORMAL", 2000);
    Test.expectNoText("KEYBINDINGS");

    // 3. Toggle Dotfiles
    Test.press(".");
    Test.waitForText(".git", 2000);
    Test.expectText(".env");

    // 4. Cycle Sort Mode
    Test.press("s");
    Test.waitForText("Sort: size", 2000);

    Test.captureText("actions.txt");
}

run();
