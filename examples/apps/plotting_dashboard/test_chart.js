function expectBrailleLine(x1, y1, x2, y2, minimumColumns, chartName) {
    let brailleCount = 0
    let coveredColumns = {}
    const rows = Test.frameText().split("\n")
    for (let y = y1; y <= y2; ++y) {
        for (let x = x1; x <= x2; ++x) {
            const character = rows[y].charAt(x)
            if (!character)
                continue

            const code = character.charCodeAt(0)
            if (code < 0x2800 || code > 0x28ff)
                continue

            ++brailleCount
            coveredColumns[x] = true

            let dots = code - 0x2800
            let dotCount = 0
            while (dots !== 0) {
                dotCount += dots & 1
                dots >>= 1
            }
            if (dotCount > 2)
                Test.fail(chartName + " contains an overdrawn Braille glyph at " + x + "," + y)
        }
    }

    if (brailleCount === 0)
        Test.fail(chartName + " did not render any Braille samples")
    if (Object.keys(coveredColumns).length < minimumColumns)
        Test.fail(chartName + " is too sparse across the X axis")
}

Test.expectText("Qtui chart demo")
Test.expectText("Bar chart")
Test.expectText("Line chart")
Test.expectText("Scatter chart")
Test.expectText("RUNNING")
Test.expectText("75 000")
Test.expectRegex("[⠁-⣿]")
Test.expectRegex("└─+┴─+")

if (Test.cell(8, 4).character !== "┌")
    Test.fail("The animated chart Y axis does not start with a cell-aligned corner")
if (Test.cell(8, 9).character !== "├")
    Test.fail("The animated chart Y axis is missing its middle tick")
if (Test.cell(39, 14).character !== "┴")
    Test.fail("The animated chart X axis is missing its middle tick")

expectBrailleLine(9, 4, 68, 13, 50, "Animated chart")
expectBrailleLine(9, 23, 56, 34, 25, "Line chart")

Test.captureText("initial.txt")

Test.injectKey("Space")
Test.expectRepaint()
Test.expectText("PAUSED")
Test.captureText("paused.txt")

Test.injectKey("Space")
Test.expectRepaint()
Test.expectText("RUNNING")

const frameBeforeAnimation = Test.frameCount()
Test.sleep(600)
if (Test.frameCount() <= frameBeforeAnimation)
    Test.fail("The chart did not repaint while its animation was running")

expectBrailleLine(9, 4, 68, 13, 50, "Animated chart after repaint")
Test.captureText("animated.txt")
