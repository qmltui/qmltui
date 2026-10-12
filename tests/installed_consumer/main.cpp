// The generated list of every header this Qamui configuration installs; the
// custom item below then exercises three of them.
#include QAMUI_PUBLIC_HEADERS

#include <Qamui/Core/Items/Item.h>
#include <Qamui/Core/Primitives/Color.h>
#include <Qamui/Core/Primitives/TextGridBuffer.h>

class CustomItem final : public qamui::Item
{
public:
    using Item::Item;

    void paintForTest(qamui::TextGridBuffer &buffer, int absX, int absY)
    {
        paint(buffer, absX, absY, {});
    }

protected:
    void paint(qamui::TextGridBuffer &buffer, int absX, int absY,
               const qamui::ClipRect &) const override {
        if (qamui::Cell *cell = buffer.tryCellAt(absX, absY)) {
            cell->setCharacter("X");
            cell->setForeground(qamui::Color("tomato"));
        }
    }
};

int main()
{
    qamui::TextGridBuffer buffer;
    if (!buffer.resize(1, 1))
        return 1;

    CustomItem item;
    item.setWidth(1);
    item.setHeight(1);
    item.paintForTest(buffer, 0, 0);

    const qamui::Cell *cell = buffer.tryCellAt(0, 0);
    if (!cell || cell->character() != "X")
        return 2;
    if (cell->foreground() != qamui::Color("tomato"))
        return 3;
    if (qamui::braille::utf8(0xFF).size() != 3)
        return 4;

    bool receivedDiagnostic = false;
    auto routing = qamui::QtMessageRouting::install([&](const qamui::QtMessage &msg) {
        if (msg.message.contains(QLatin1String("installed-consumer-probe")))
            receivedDiagnostic = true;
    });
    if (!routing)
        return 5;
    qInfo("installed-consumer-probe");
    if (!receivedDiagnostic)
        return 6;
    routing->reset();

    qamui::TextArea textArea;
    textArea.setText(QStringLiteral("hello world"));
    qamui::HighlightRange range(0, 5, qamui::Color("blue"));
    if (!textArea.setHighlightLayer(QStringLiteral("test"), {range}, textArea.textRevision()))
        return 7;
    if (range.color() != qamui::Color("blue"))
        return 8;

    return 0;
}
