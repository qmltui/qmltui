function main(terminal) {
    const samples = []
    for (let index = 0; index < 30; ++index) {
        const started = Date.now()
        terminal.press(index % 2 === 0 ? "F2" : "F1")
        samples.push(Date.now() - started)
    }

    samples.sort((left, right) => left - right)
    const p95 = samples[Math.ceil(samples.length * 0.95) - 1]
    terminal.captureText(`starter_template/local-p95-${p95}.txt`)
    if (p95 > 20)
        terminal.fail(`Local key-to-frame p95 was ${p95} ms, expected at most 20 ms`)

    terminal.expectText("[F1] Network")
    terminal.captureText("starter_template/latency-final.txt")
}
