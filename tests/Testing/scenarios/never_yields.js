// A scenario that never returns to the event loop. The QTimer watchdog cannot
// fire while this runs, so the hard watchdog has to interrupt the JS engine.
Test.expectText("TESTRUNNER FIXTURE");
while (true) {
}
