function run() {
    Test.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve) {
        Test.sleep(50);
        Test.captureText("async_success.txt");
        resolve();
    });
}
run();
