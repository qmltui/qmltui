function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    return new Promise(function(resolve, reject) {
        app.sleep(50);
        reject(new Error("intentional async error"));
    });
}
