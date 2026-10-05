// test_search.js - Tests instant live filtering and search mode

function run() {
    Test.waitForText("NORMAL", 2000);
    Test.expectText("README.md");
    Test.expectText("CMakeLists.txt");

    // Enter Filter Mode
    Test.press("/");
    Test.waitForText("FILTER", 2000);
    Test.expectText("Filter:");

    // Type query
    Test.typeText("CMake");
    Test.waitForText("CMakeLists.txt", 2000);
    Test.expectNoText("README.md");
    Test.expectNoText("package.json");

    // Commit filter with Enter
    Test.press("Enter");
    Test.expectText("CMakeLists.txt");

    // Clear filter with Escape
    Test.press("Escape");
    Test.waitForText("README.md", 2000);
    Test.expectText("package.json");

    Test.captureText("search.txt");
}

run();
