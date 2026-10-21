/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Calendar & Day Notes", 2000);
    terminal.expectText("Jump & Navigate");
    terminal.expectText("Selected Date");
    terminal.expectText("Day Notes & Reminders");
    terminal.expectText("STATUS:");

    terminal.captureText("initial_frame.txt");

    terminal.press("Right");
    terminal.expectText("STATUS:");

    terminal.press("Down");
    terminal.expectText("STATUS:");

    terminal.press("t");
    terminal.waitForText("TODAY", 2000);

    terminal.press("]");
    terminal.waitForText("2027", 2000);

    terminal.press("[");
    terminal.waitForText("2026", 2000);

    terminal.clickText("DD/MM/YYYY");
    terminal.typeText("25/12/2026");
    terminal.press("Return");
    terminal.waitForText("25/12/2026", 2000);
    terminal.expectText("Holiday release freeze");

    terminal.clickText("Add note...");
    terminal.typeText("Publish changelog");
    terminal.press("Return");
    terminal.waitForText("Publish changelog", 2000);

    terminal.captureText("after_note.txt");
}
