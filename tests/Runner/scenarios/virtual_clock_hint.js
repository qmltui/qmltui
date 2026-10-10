// A wait that depends on paused time must fail fast and name the cure.
function main(terminal) {
    terminal.waitForText("STATUS: ASYNC READY", 300);
    terminal.fail("The 75 ms Timer cannot fire while the virtual clock is paused");
}
