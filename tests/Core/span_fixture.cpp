#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/Cell.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>

namespace {

// Minimal custom Item emitting a Strikethrough cell to verify Runner::expectStyle().
class StruckCellItem : public qmltui::Item {
public:
    using Item::Item;

protected:
    void paint(qmltui::TextGridBuffer& buffer, int absX, int absY,
               const qmltui::ClipRect& clipRect) const override {
        if (!clipRect.contains(absX, absY)) return;
        if (qmltui::Cell* cell = buffer.tryCellAt(absX, absY)) {
            cell->setCharacter("X");
            cell->attributes = qmltui::TextAttribute::Strikethrough;
        }
    }
};

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qmlRegisterType<StruckCellItem>("SpanFixture", 1, 0, "StruckCellItem");
    QQmlApplicationEngine engine;
    engine.loadData(R"(
        pragma ComponentBehavior: Bound
        import QtQml
        import QmlTui
        import SpanFixture
        Terminal {
            Text {
                id: label
                text: "Fallback"
                color: "#00ffff"
                bold: true
                spans: [ { text: "Status: ", bold: false, dim: true },
                         QmlTui.span("Active\n", { color: "#00ff00" }),
                         { text: "Workers: 4" } ]
            }
            StruckCellItem {
                id: struckProbe
                x: 0
                y: 2
                width: 1
                height: 1
                visible: false
            }
            Keys.onPressed: event => {
                if (event.key === Qt.Key_S) {
                    label.color = "#ff0000"
                    label.bold = false
                } else if (event.key === Qt.Key_R) {
                    label.spans = [{ text: "Replaced", underline: true }]
                } else if (event.key === Qt.Key_K) {
                    struckProbe.visible = true
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
