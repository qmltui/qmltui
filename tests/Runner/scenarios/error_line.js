// The function wrapper must not move line numbers: the error below is thrown
// on line 7 and must be reported there.
function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    const distraction = 1;
    void distraction;
    throw new Error("deliberate error on line seven");
}
