Test.waitForText("Welcome to Qtui Markdown Viewer");
Test.expectTextInRegion(0, 0, 70, 3, "Welcome to Qtui Markdow");
Test.expectNoText("OUTLINE");
Test.expectRegex("(^|\\n)\\s*1\\s+│");

// Tab and Backtab navigate headings in both directions and wrap at the ends.
Test.press("Tab");
Test.expectText("Section: Key Features");
Test.press("Backtab");
Test.expectText("Section: Welcome to Qtui Markdown Viewer");
Test.press("Backtab");
Test.expectText("Section: Keyboard Shortcuts Reference");
Test.press("Tab");
Test.expectText("Section: Welcome to Qtui Markdown Viewer");

// The line-number gutter must not displace link hit-testing.
Test.clickText("md4qt");
Test.expectText("Activated Link: https://github.com/KDE/md4qt");

// A whitespace-only query used to loop forever. It must settle as no matches.
Test.press("/");
Test.typeText("   ");
Test.waitForText("No matches");
Test.press("Backspace");
Test.press("Backspace");
Test.press("Backspace");

// Verify exact match highlighting and palette changes after a theme switch.
Test.typeText("Table");
Test.waitForText("Match 1 of");
let tableMatches = Test.findText("Table");
if (tableMatches.length === 0)
    Test.fail("Search did not scroll the active Table match into view");
const beforeTheme = Test.cell(tableMatches[0].x, tableMatches[0].y).background;
if (beforeTheme === "#00000000")
    Test.fail("Active search match has no highlight background");
Test.press("Escape");
Test.expectNoText("Match 1/");

Test.press("t");
Test.waitForText("dracula");
Test.press("/");
Test.typeText("Table");
Test.waitForText("Match 1 of");
tableMatches = Test.findText("Table");
const afterTheme = Test.cell(tableMatches[0].x, tableMatches[0].y).background;
if (afterTheme === beforeTheme)
    Test.fail("Search highlight did not follow the active theme");
Test.press("Escape");

// Exercise recursive AST traversal and Unicode-safe narrow rendering.
Test.press("o");
Test.press("o");
Test.press("o");
Test.waitForText("Linked Unicode Heading");
Test.expectText("nested list item");
Test.expectText("const auto answer = 42;");

Test.captureText("markdown_viewer_final.txt");
