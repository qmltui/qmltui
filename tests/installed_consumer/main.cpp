#include <Color.h>
#include <Item.h>
#include <TextGridBuffer.h>

class CustomItem final : public qtui::Item
{
public:
    using Item::Item;

    void paintForTest(qtui::TextGridBuffer &buffer, int absX, int absY)
    {
        paint(buffer, absX, absY, {});
    }

protected:
    void paint(qtui::TextGridBuffer &buffer, int absX, int absY,
               const qtui::ClipRect &) const override {
        if (qtui::Cell *cell = buffer.tryCellAt(absX, absY)) {
            cell->setCharacter("X");
            cell->setForeground(qtui::Color("tomato"));
        }
    }
};

int main()
{
    qtui::TextGridBuffer buffer;
    if (!buffer.resize(1, 1))
        return 1;

    CustomItem item;
    item.setWidth(1);
    item.setHeight(1);
    item.paintForTest(buffer, 0, 0);

    const qtui::Cell *cell = buffer.tryCellAt(0, 0);
    if (!cell || cell->character() != "X")
        return 2;
    if (cell->foreground() != qtui::Color("tomato"))
        return 3;
    return 0;
}
