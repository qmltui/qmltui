// The generated list of every header this QmlTui configuration installs; the
// custom item below then exercises three of them.
#include QMLTUI_PUBLIC_HEADERS

#include <QCoreApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <memory>

static_assert(sizeof(qmltui::Item) == sizeof(QObject) + sizeof(std::unique_ptr<void*>),
              "Item must hold only its d_ptr beyond QObject for binary ABI stability");
static_assert(sizeof(qmltui::FocusScope) == sizeof(qmltui::Item) + sizeof(std::unique_ptr<void*>),
              "FocusScope must hold only its d_ptr beyond Item for binary ABI stability");

class CustomItem final : public qmltui::Item {
public:
    using Item::Item;

    void paintForTest(qmltui::TextGridBuffer& buffer, int absX, int absY) {
        paint(buffer, absX, absY, {});
    }

protected:
    void paint(qmltui::TextGridBuffer& buffer, int absX, int absY,
               const qmltui::ClipRect&) const override {
        if (qmltui::Cell* cell = buffer.tryCellAt(absX, absY)) {
            cell->setCharacter("X");
            cell->setForeground(qmltui::Color("tomato"));
        }
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    qmltui::TextGridBuffer buffer;
    if (!buffer.resize(1, 1)) return 1;

    CustomItem item;
    item.setWidth(1);
    item.setHeight(1);
    item.paintForTest(buffer, 0, 0);

    const qmltui::Cell* cell = buffer.tryCellAt(0, 0);
    if (!cell || cell->character() != "X") return 2;
    if (cell->foreground() != qmltui::Color("tomato")) return 3;
    if (qmltui::braille::utf8(0xFF).size() != 3) return 4;

    bool receivedDiagnostic = false;
    auto routing = qmltui::QtMessageRouting::install([&](const qmltui::QtMessage& msg) {
        if (msg.message.contains(QLatin1String("installed-consumer-probe")))
            receivedDiagnostic = true;
    });
    if (!routing) return 5;
    qInfo("installed-consumer-probe");
    if (!receivedDiagnostic) return 6;
    routing->reset();

    qmltui::TextArea textArea;
    textArea.setText(QStringLiteral("hello world"));
    qmltui::HighlightRange range(0, 5, qmltui::Color("blue"));
    if (!textArea.setHighlightLayer(QStringLiteral("test"), {range}, textArea.textRevision()))
        return 7;
    if (range.color() != qmltui::Color("blue")) return 8;

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(QMLTUI_QML_IMPORT_PATH));
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral("import QtQml\nimport QmlTui\nimport QmlTui.Controls\n"
                                        "Terminal { Button { objectName: 'btn'; text: 'OK' } }\n"),
                      QUrl());
    std::unique_ptr<QObject> root(component.create());
    if (!root || !root->findChild<QObject*>(QStringLiteral("btn"))) return 9;

    return 0;
}
