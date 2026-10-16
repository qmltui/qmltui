/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("StackView Wizard");
    terminal.expectText("Step 1: Environment");
    terminal.expectText("stack.depth: 1");
    terminal.sleep(300);

    terminal.press("n");
    terminal.expectText("Step 2: Replicas");
    terminal.expectText("stack.depth: 2");
    terminal.sleep(300);

    terminal.press("n");
    terminal.expectText("Step 3: Confirm");
    terminal.expectText("stack.depth: 3");
    terminal.sleep(300);

    terminal.press("p");
    terminal.expectText("Step 2: Replicas");
    terminal.expectText("stack.depth: 2");
    terminal.sleep(300);

    terminal.press("r");
    terminal.expectText("Step 1: Environment");
    terminal.expectText("stack.depth: 1");
    terminal.sleep(300);
}
