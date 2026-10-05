// test_calendar.js - Verification of Year Calendar & Day Planner

function run() {
    // 1. Initial screen assertions
    Test.waitForText("YEAR CALENDAR & PLANNER", 2000);
    Test.expectText("Jump & Navigate");
    Test.expectText("Selected Date");
    Test.expectText("Day Notes & Reminders");
    Test.expectText("STATUS:");

    // Capture initial frame
    Test.captureText("initial_frame.txt");

    // 2. Navigation with Arrow keys
    Test.press("Right");
    Test.expectText("STATUS:");

    Test.press("Down");
    Test.expectText("STATUS:");

    // 3. Jump to Today with 'T'
    Test.press("t");
    Test.waitForText("TODAY", 2000);

    // 4. Year navigation with '[' and ']'
    Test.press("]");
    Test.waitForText("2027", 2000);

    Test.press("[");
    Test.waitForText("2026", 2000);

    // 5. Theme switching with keys 1-4
    Test.press("2");
    Test.waitForText("Amber Glow", 2000);

    Test.press("3");
    Test.waitForText("Emerald Matrix", 2000);

    Test.press("4");
    Test.waitForText("Nord Frost", 2000);

    Test.press("1");
    Test.waitForText("Cyber Cyan", 2000);

    // 6. Test Column mode cycling with 'c'
    Test.press("c");
    Test.waitForText("Grid: 3 columns", 2000);

    Test.press("c");
    Test.waitForText("Grid: 4 columns", 2000);

    Test.press("c");
    Test.waitForText("Grid: 2 columns", 2000);

    // 7. Test Date Jump: Click input, type date DD/MM/YYYY and press Return
    Test.clickText("DD/MM/YYYY");
    Test.typeText("25/12/2026");
    Test.press("Return");
    Test.waitForText("25/12/2026", 2000);
    Test.expectText("Holiday Celebration");

    // 8. Add a new free-form note to the selected day
    Test.clickText("Add note...");
    Test.typeText("Buy Christmas gifts");
    Test.press("Return");
    Test.waitForText("Buy Christmas gifts", 2000);

    // Capture frame after adding note
    Test.captureText("after_note.txt");
}

run();
