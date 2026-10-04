function run() {
    Test.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve, reject) {
        Test.sleep(50);
        reject(new Error("intentional async error"));
    });
}
run();
