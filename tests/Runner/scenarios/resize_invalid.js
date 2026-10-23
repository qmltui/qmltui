function main(app) {
    app.resize(0, 10);
    app.fail("resize(0, 10) must fail the scenario before this line");
}
