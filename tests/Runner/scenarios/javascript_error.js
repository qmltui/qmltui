function main(app) {
    app.finish();
    throw new Error("intentional JavaScript failure after finish");
}
