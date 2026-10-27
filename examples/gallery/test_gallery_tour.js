/** @param {Runner} app */
function main(app) {
    function toScenarioFile(qmlFile) {
        const stem = String(qmlFile).replace(/\.qml$/i, "");
        return stem.replace(/([a-z0-9])([A-Z])/g, "$1_$2").toLowerCase() + ".js";
    }

    const demoApp = Object.create(app);
    demoApp.stopRecording = function() {};
    demoApp.captureText = function() {};
    demoApp.sleep = function(ms) {
        app.sleep(ms);
    };
    demoApp.clickText = function(text, a, b, c) {
        const host = app.item("demoHost");
        if (host.findText(text).length > 0) {
            if (arguments.length === 1)
                host.clickText(text);
            else if (arguments.length === 2)
                host.clickText(text, a);
            else if (arguments.length === 3)
                host.clickText(text, a, b);
            else
                host.clickText(text, a, b, c);
            return;
        }
        if (arguments.length === 1)
            app.clickText(text);
        else if (arguments.length === 2)
            app.clickText(text, a);
        else if (arguments.length === 3)
            app.clickText(text, a, b);
        else
            app.clickText(text, a, b, c);
    };

    app.expectText("QmlTui Gallery");
    app.marker("Gallery");
    app.captureText("gallery/grid.txt");
    app.sleep(1200);

    const root = app.rootObject();
    const catalog = root.catalog;
    const scenarioSources = root.scenarioSources || {};

    for (let i = 0; i < catalog.length; ++i) {
        const entry = catalog[i];
        app.press("Enter");
        app.expectText("back to grid");
        app.expectNoText("✗ Error");
        app.marker(entry.name);
        app.sleep(600);

        const scenarioFile = toScenarioFile(entry.file);
        const source = scenarioSources[scenarioFile];
        if (typeof source === "string" && source.length > 0) {
            const runDemo = new Function("app", source + "\nreturn main(app);");
            runDemo(demoApp);
        } else {
            app.sleep(1200);
        }

        app.sleep(400);
        app.captureText("gallery/page_" + i + ".txt");
        app.press("Escape");
        app.expectText("Components (");
        app.sleep(400);
        if (i + 1 < catalog.length) {
            app.press("Right");
            app.sleep(600);
        }
    }

    app.marker("Tour complete");
    app.sleep(1500);
    app.stopRecording();
}
