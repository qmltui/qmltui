/** @param {Runner} app */
function main(app) {
    app.expectText("INLINE FIXTURE");
    // In a 24-row viewport, the 2-row Inline prompt sits near the bottom
    // (mouseRowOffset > 0), so item() and region() clicks must translate frame Y.
    app.item("inlineTitle").click();
    app.expectText("INLINE CLICKED 1");
    app.region(0, 0, 30, 1).clickText("CLICKED");
    app.expectText("INLINE CLICKED 2");
}
