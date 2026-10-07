// test_search.js - Tests instant live filtering and search mode

function main(terminal) {
    terminal.waitForText("NORMAL", 2000);
    terminal.expectText("README.md");
    terminal.expectText("CMakeLists.txt");

    // Enter Filter Mode
    terminal.press("/");
    terminal.waitForText("FILTER", 2000);
    terminal.expectText("Filter:");

    // Type query
    terminal.typeText("CMake");
    terminal.waitForText("CMakeLists.txt", 2000);
    terminal.expectNoText("README.md");
    terminal.expectNoText("package.json");

    // Commit filter with Enter
    terminal.press("Enter");
    terminal.expectText("CMakeLists.txt");

    // Clear filter with Escape
    terminal.press("Escape");
    terminal.waitForText("README.md", 2000);
    terminal.expectText("package.json");

    terminal.captureText("search.txt");
}
