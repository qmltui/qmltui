// The styled capture records colors and attributes of committed rows.
function main(app) {
    app.expectText("INLINE FIXTURE");
    app.press("F3");
    app.waitForScrollbackText("STYLED ROW");
    app.captureScrollbackStyled("styled.txt");
}
