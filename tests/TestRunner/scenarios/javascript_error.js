function main(terminal) {
    terminal.finish();
    throw new Error("intentional JavaScript failure after finish");
}
