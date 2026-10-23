// A module scenario: main is an export, and helpers come from imports.
import { titleText, typeGreeting } from "./lib/module_helpers.mjs";

export function main(app) {
    app.waitForText(titleText);
    typeGreeting(app);
    app.waitForText("INPUT: modules work");
    app.captureText("module_main.txt");
}
