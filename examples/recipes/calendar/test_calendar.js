/** @param {Runner} app */
function main(app) {
    app.waitForText("CALENDAR & DAY PLANNER", 2000);
    app.expectText("Jump & Navigate");
    app.expectText("Selected Date");
    app.expectText("Day Notes & Reminders");
    app.expectText("STATUS:");

    app.captureText("initial_frame.txt");
    app.sleep(700);

    app.press("Right");
    app.expectText("STATUS:");
    app.sleep(600);

    app.press("Down");
    app.expectText("STATUS:");
    app.sleep(600);

    app.press("t");
    app.waitForText("TODAY", 2000);
    app.sleep(700);

    app.press("]");
    app.waitForText("2027", 2000);
    app.sleep(600);

    app.press("[");
    app.waitForText("2026", 2000);
    app.sleep(600);

    app.clickText("DD/MM/YYYY");
    app.typeText("25/12/2026");
    app.press("Return");
    app.waitForText("25/12/2026", 2000);
    app.expectText("Holiday release freeze");
    app.sleep(800);

    app.clickText("Add note...");
    app.typeText("Publish changelog");
    app.press("Return");
    app.waitForText("Publish changelog", 2000);
    app.sleep(900);

    app.captureText("after_note.txt");
}
