#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import Qtui
        Terminal {
            Text {
                id: label
                text: "Fallback"
                color: "#00ffff"
                bold: true
                spans: [ { text: "Status: ", bold: false, dim: true },
                         Qtui.span("Active\n", { color: "#00ff00" }),
                         { text: "Workers: 4" } ]
            }
            Keys.onPressed: event => {
                if (event.key === Qt.Key_S) {
                    label.color = "#ff0000"
                    label.bold = false
                } else if (event.key === Qt.Key_R) {
                    label.spans = [{ text: "Replaced", underline: true }]
                } else if (event.key === Qt.Key_C) {
                    label.spans = []
                } else if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
                    Qt.quit()
                } else {
                    return
                }
                event.accepted = true
            }
        }
    )");
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
