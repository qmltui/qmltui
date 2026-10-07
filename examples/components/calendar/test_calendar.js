// test_calendar.js - Verification of Year Calendar & Day Planner

function main(terminal) {
    // 1. Initial screen assertions
    terminal.waitForText("YEAR CALENDAR & PLANNER", 2000);
    terminal.expectText("Jump & Navigate");
    terminal.expectText("Selected Date");
    terminal.expectText("Day Notes & Reminders");
    terminal.expectText("STATUS:");

    // Capture initial frame
    terminal.captureText("initial_frame.txt");

    // 2. Navigation with Arrow keys
    terminal.press("Right");
    terminal.expectText("STATUS:");

    terminal.press("Down");
    terminal.expectText("STATUS:");

    // 3. Jump to Today with 'T'
    terminal.press("t");
    terminal.waitForText("TODAY", 2000);

    // 4. Year navigation with '[' and ']'
    terminal.press("]");
    terminal.waitForText("2027", 2000);

    terminal.press("[");
    terminal.waitForText("2026", 2000);

    // 5. Theme switching with keys 1-4
    terminal.press("2");
    terminal.waitForText("Amber Glow", 2000);

    terminal.press("3");
    terminal.waitForText("Emerald Matrix", 2000);

    terminal.press("4");
    terminal.waitForText("Nord Frost", 2000);

    terminal.press("1");
    terminal.waitForText("Cyber Cyan", 2000);

    // 6. Test Column mode cycling with 'c'
    terminal.press("c");
    terminal.waitForText("Grid: 3 columns", 2000);

    terminal.press("c");
    terminal.waitForText("Grid: 4 columns", 2000);

    terminal.press("c");
    terminal.waitForText("Grid: 2 columns", 2000);

    // 7. Test Date Jump: Click input, type date DD/MM/YYYY and press Return
    terminal.clickText("DD/MM/YYYY");
    terminal.typeText("25/12/2026");
    terminal.press("Return");
    terminal.waitForText("25/12/2026", 2000);
    terminal.expectText("Holiday Celebration");

    // 8. Add a new free-form note to the selected day
    terminal.clickText("Add note...");
    terminal.typeText("Buy Christmas gifts");
    terminal.press("Return");
    terminal.waitForText("Buy Christmas gifts", 2000);

    // Capture frame after adding note
    terminal.captureText("after_note.txt");
}
