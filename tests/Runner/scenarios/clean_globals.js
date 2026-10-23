// Top-level declarations of a classic scenario must stay inside the script's
// own scope instead of becoming properties of the application's JS globals.
function helper() { return 1; }
var topLevelVar = 2;

function main(app) {
    // QJSEngine has no globalThis; a non-strict function's this is the global object.
    const globals = (function() { return this; })();
    if (typeof globals.helper !== "undefined")
        app.fail("helper() leaked into the global object");
    if (typeof globals.topLevelVar !== "undefined")
        app.fail("topLevelVar leaked into the global object");
    if (typeof globals.main !== "undefined")
        app.fail("main() leaked into the global object");
    if (helper() !== 1 || topLevelVar !== 2)
        app.fail("the scenario lost access to its own scope");
    app.expectText("TESTRUNNER FIXTURE");
}
