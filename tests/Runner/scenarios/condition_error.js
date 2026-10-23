function main(app) {
    app.waitForCondition(() => {
        throw new Error("intentional condition failure");
    }, 100);
}
