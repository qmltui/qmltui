function run() {
    Test.waitForText("STATUS: ASYNC READY");
    Test.expectSnapshot("snapshots/fixture.txt");
}
run();
