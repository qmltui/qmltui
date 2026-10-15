/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Ask anything")
    terminal.expectText("[F2] model")

    // Hovering beyond the first three commands used to shift the window and
    // delete the MouseArea while its onEntered handler was still executing.
    // Mouse selection must keep the command under the pointer stationary.
    terminal.typeText("/")
    terminal.hoverText("/demo 3")
    terminal.expectText("Commands [4/13]")
    terminal.expectText("/help ·")
    terminal.hoverText("/demo 4")
    terminal.expectText("Commands [5/13]")
    terminal.captureText("mouse_autocomplete_hover.txt")
    // Keyboard navigation still scrolls the window; its rows remain usable
    // by the mouse afterwards, including clicking a nonselected command.
    terminal.press("Down")
    terminal.expectText("Commands [6/13]")
    terminal.expectNoText("/help ·")
    terminal.hoverText("/tree")
    terminal.expectText("Commands [8/13]")
    terminal.clickText("/model")
    terminal.expectText("Neural model selector")
    terminal.press("Escape")
    terminal.expectText("Ask anything")

    // 1. Mouse click on [F2] model opens model picker modal
    terminal.clickText("[F2] model")
    terminal.expectText("Neural model selector")
    terminal.captureText("mouse_model_modal.txt")

    // Test hover on bottom model row (DeepSeek R1)
    terminal.hoverText("DeepSeek R1")
    terminal.expectText("❯ [4]")

    // Test hover on second model row (GPT-4.5 Preview)
    terminal.hoverText("GPT-4.5 Preview")
    terminal.expectText("❯ [2]")

    // 2. Mouse click on model entry selects it and closes modal
    terminal.clickText("GPT-4.5 Preview")
    terminal.expectText("gpt-4.5-preview")
    terminal.expectText("Ask anything")

    // 3. Mouse click on [F3] diff opens diff inspector modal
    terminal.clickText("[F3] diff")
    terminal.expectText("InlineRenderer.cpp")
    terminal.captureText("mouse_diff_modal.txt")

    // 4. Mouse click on close text dismisses diff modal
    terminal.clickText("close")
    terminal.expectText("Ask anything")

    // 5. Test autocomplete command palette click
    terminal.typeText("/dem")
    terminal.expectText("/demo 1")
    terminal.clickText("/demo 1")

    // 6. Wait for permission prompt and click permission choice
    terminal.advanceTime(4000)
    terminal.waitForText("Allow once", 7000)
    terminal.expectText("Deny execution")
    terminal.captureText("mouse_permission.txt")

    // Click "Deny execution" option
    terminal.clickText("Deny execution")
    terminal.expectText("Ask anything")
    terminal.expectScrollbackText("Execution denied")

    // 7. Start demo 4 and click abort with mouse while working
    terminal.typeText("/demo 4")
    terminal.press("Enter")
    terminal.waitForText("to abort", 2000)
    terminal.clickText("to abort")
    terminal.expectText("Ask anything")
    terminal.expectScrollbackText("Execution interrupted")
    terminal.captureText("mouse_interrupted.txt")
}
