function showStep() {
    Test.sleep(2000)
}

Test.waitForCondition((frameText) =>
    frameText.includes("[■ STANDBY]")
    && frameText.includes("STATUS: READY FOR OPERATOR COMMANDS"), 3000)
Test.captureText("plc/standby.txt")
showStep()

Test.press("S")
Test.waitForCondition((frameText) =>
    frameText.includes("[● RUNNING]")
    && frameText.includes("STATE: PROCESSING (START CMD)"), 3000)
showStep()

Test.press("F2")
Test.waitForCondition((frameText) =>
    frameText.includes("ALARM HISTORY")
    && frameText.includes("[INFO]")
    && frameText.includes("STATE: PROCESSING (START CMD)"), 3000)

Test.press("M")
Test.waitForText("MAINTENANCE MODE")
showStep()
Test.press("Y")
Test.waitForCondition((frameText) =>
    frameText.includes("[⚙ MAINT]")
    && frameText.includes("STATE: MAINTENANCE ACTIVE"), 3000)
Test.captureText("plc/maintenance.txt")
showStep()

Test.press("M")
Test.waitForText("Exit maintenance mode")
Test.press("N")
Test.waitForText("[⚙ MAINT]")
Test.expectNoText("CONFIRM ACTION")

Test.press("M")
Test.waitForText("Exit maintenance mode")
Test.press("Y")
Test.waitForCondition((frameText) =>
    frameText.includes("[■ STANDBY]")
    && frameText.includes("STATE: IDLE (MAINT RUN COMPLETED)"), 3000)

Test.press("F3")
Test.waitForCondition((frameText) =>
    frameText.includes("PLC HARDWARE & SUBSYSTEM DIAGNOSTICS")
    && frameText.includes("CRC Error Counter"), 3000)
Test.captureText("plc/diagnostics.txt")
showStep()

Test.press("F4")
Test.waitForText("SYSTEM PARAMETERS & SAFETY THRESHOLDS")
Test.press("Tab")
Test.press("Tab")
Test.press("A", ["Ctrl"])
Test.typeText("oops")
Test.press("Enter")
Test.waitForText("REJECTED TEMP LIMIT: oops")

Test.press("A", ["Ctrl"])
Test.typeText("30")
Test.press("Enter")
Test.waitForText("LIMIT TEMP -> 30.0C")

Test.press("F1")
Test.press("S")
Test.waitForCondition((frameText) =>
    frameText.includes("[▲ FAULT!]")
    && frameText.includes("[V-A: OFF]")
    && frameText.includes("[V-B: OFF]"), 5000)
showStep()

Test.press("F2")
Test.waitForCondition((frameText) =>
    frameText.includes("[CRITICAL]")
    && frameText.includes("OVER-TEMPERATURE FAULT"), 3000)

Test.press("R")
Test.waitForText("CLEAR FAULT ALARM")
Test.press("N")
Test.waitForText("[▲ FAULT!]")
Test.expectNoText("CONFIRM ACTION")

Test.press("R")
Test.waitForText("CLEAR FAULT ALARM")
Test.press("Y")
Test.waitForCondition((frameText) =>
    frameText.includes("[■ STANDBY]")
    && frameText.includes("STATE: IDLE (ALARM RESET)"), 3000)
Test.expectNoText("[▲ FAULT!]")
Test.captureText("plc/reset.txt")
showStep()

Test.finish()
