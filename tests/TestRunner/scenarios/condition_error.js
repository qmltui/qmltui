function main(terminal) {
    terminal.waitForCondition(() => {
        throw new Error("intentional condition failure");
    }, 100);
}
