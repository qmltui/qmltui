// A module scenario: main is an export, and helpers come from imports.
import { titleText, typeGreeting } from "./lib/module_helpers.mjs";

export function main(terminal) {
    terminal.waitForText(titleText);
    typeGreeting(terminal);
    terminal.waitForText("INPUT: modules work");
    terminal.captureText("module_main.txt");
}
