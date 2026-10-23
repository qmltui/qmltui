// A wait that depends on paused time must fail fast and name the cure.
function main(app) {
    app.waitForText("STATUS: ASYNC READY", 300);
    app.fail("The 75 ms Timer cannot fire while the virtual clock is paused");
}
