function main(terminal) {
    function showStep() {
        terminal.sleep(2000)
    }

    terminal.waitForCondition((frameText) =>
        frameText.includes("[■ STANDBY]")
        && frameText.includes("STATUS: READY FOR OPERATOR COMMANDS"), 3000)
    terminal.captureText("plc/standby.txt")
    showStep()

    terminal.press("S")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[● RUNNING]")
        && frameText.includes("STATE: PROCESSING (START CMD)"), 3000)
    showStep()

    terminal.press("F2")
    terminal.waitForCondition((frameText) =>
        frameText.includes("ALARM HISTORY")
        && frameText.includes("[INFO]")
        && frameText.includes("STATE: PROCESSING (START CMD)"), 3000)

    terminal.press("M")
    terminal.waitForText("MAINTENANCE MODE")
    showStep()
    terminal.press("Y")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[⚙ MAINT]")
        && frameText.includes("STATE: MAINTENANCE ACTIVE"), 3000)
    terminal.captureText("plc/maintenance.txt")
    showStep()

    terminal.press("M")
    terminal.waitForText("Exit maintenance mode")
    terminal.press("N")
    terminal.waitForText("[⚙ MAINT]")
    terminal.expectNoText("CONFIRM ACTION")

    terminal.press("M")
    terminal.waitForText("Exit maintenance mode")
    terminal.press("Y")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[■ STANDBY]")
        && frameText.includes("STATE: IDLE (MAINT RUN COMPLETED)"), 3000)

    terminal.press("F3")
    terminal.waitForCondition((frameText) =>
        frameText.includes("PLC HARDWARE & SUBSYSTEM DIAGNOSTICS")
        && frameText.includes("CRC Error Counter"), 3000)
    terminal.captureText("plc/diagnostics.txt")
    showStep()

    terminal.press("F4")
    terminal.waitForText("SYSTEM PARAMETERS & SAFETY THRESHOLDS")
    terminal.press("Tab")
    terminal.press("Tab")
    terminal.press("A", ["Ctrl"])
    terminal.typeText("oops")
    terminal.press("Enter")
    terminal.waitForText("REJECTED TEMP LIMIT: oops")

    terminal.press("A", ["Ctrl"])
    terminal.typeText("30")
    terminal.press("Enter")
    terminal.waitForText("LIMIT TEMP -> 30.0C")

    terminal.press("F1")
    terminal.press("S")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[▲ FAULT!]")
        && frameText.includes("[V-A: OFF]")
        && frameText.includes("[V-B: OFF]"), 5000)
    showStep()

    terminal.press("F2")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[CRITICAL]")
        && frameText.includes("OVER-TEMPERATURE FAULT"), 3000)

    terminal.press("R")
    terminal.waitForText("CLEAR FAULT ALARM")
    terminal.press("N")
    terminal.waitForText("[▲ FAULT!]")
    terminal.expectNoText("CONFIRM ACTION")

    terminal.press("R")
    terminal.waitForText("CLEAR FAULT ALARM")
    terminal.press("Y")
    terminal.waitForCondition((frameText) =>
        frameText.includes("[■ STANDBY]")
        && frameText.includes("STATE: IDLE (ALARM RESET)"), 3000)
    terminal.expectNoText("[▲ FAULT!]")
    terminal.captureText("plc/reset.txt")
    showStep()

    terminal.finish()
}
