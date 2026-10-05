// test_navigation.js - Tests Miller column navigation (j/k/h/l, Enter)

function run() {
    Test.waitForText("CURRENT", 2000);
    Test.expectText("PARENT");
    Test.expectText("PREVIEW");
    Test.expectText("NORMAL");

    // Check that top level folders and files are listed
    Test.expectText("assets");
    Test.expectText("docs");
    Test.expectText("src");
    Test.expectText("README.md");

    // Move cursor down
    Test.press("j");
    Test.press("j");

    // Navigate into 'src' folder (or directory under cursor)
    // Press 'l' or 'Enter' to enter directory
    Test.press("l");
    Test.waitForText("main.cpp", 2000);
    Test.expectText("App.cpp");
    Test.expectText("utils.rs");

    // Navigate back up to parent directory
    Test.press("h");
    Test.waitForText("README.md", 2000);
    Test.expectText("docs");
    Test.expectText("src");

    // Capture text artifact
    Test.captureText("navigation.txt");
}

run();
