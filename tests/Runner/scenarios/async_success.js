function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve) {
        app.sleep(50);
        app.captureText("async_success.txt");
        resolve();
    });
}
