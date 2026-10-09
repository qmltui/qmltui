function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve, reject) {
        terminal.sleep(50);
        reject(new Error("intentional async error"));
    });
}
