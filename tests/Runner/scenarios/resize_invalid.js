function main(terminal) {
    terminal.resize(0, 10);
    terminal.fail("resize(0, 10) must fail the scenario before this line");
}
