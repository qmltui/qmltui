function main(terminal) {
    terminal.expectText("SERVICE MONITOR")
    terminal.expectText("api-gateway")
    terminal.expectText("LATENCY")
    terminal.expectText("12 ms")
    terminal.expectText("▲ DEGRADED")
    terminal.press("Down")
    terminal.press("Down")
    terminal.press("Enter")
    terminal.expectText("Selected service row 2")
}
