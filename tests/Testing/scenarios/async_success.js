function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve) {
        terminal.sleep(50);
        terminal.captureText("async_success.txt");
        resolve();
    });
}
