#include <Qtui/Core/items/GridLayout.h>
#include <Qtui/Core/items/Layout.h>
#include <algorithm>
#include <vector>
#include <set>

namespace qtui {

GridLayout::GridLayout(QObject *parent) : Item(parent) {
    setClip(true);
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}

GridLayout::~GridLayout() = default;

void GridLayout::setColumns(int cols) {
    if (m_columns == cols) return;
    m_columns = cols;
    emit columnsChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void GridLayout::setRows(int r) {
    if (m_rows == r) return;
    m_rows = r;
    emit rowsChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void GridLayout::setColumnSpacing(int spacing) {
    if (m_columnSpacing == spacing) return;
    m_columnSpacing = spacing;
    emit columnSpacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void GridLayout::setRowSpacing(int spacing) {
    if (m_rowSpacing == spacing) return;
    m_rowSpacing = spacing;
    emit rowSpacingChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void GridLayout::setFlow(Flow flow) {
    if (m_flow == flow) return;
    m_flow = flow;
    emit flowChanged();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

struct GridItemInfo {
    Item *item = nullptr;
    int row = 0;
    int col = 0;
    int rowSpan = 1;
    int colSpan = 1;
    LayoutAttached *layout = nullptr;
};

void GridLayout::layoutChildren() {
    const auto childItems = children();
    std::vector<Item *> visibleChildren;
    for (QObject *child : childItems) {
        if (auto *item = qobject_cast<Item *>(child)) {
            if (item->visible())
                visibleChildren.push_back(item);
        }
    }

    if (visibleChildren.empty()) return;

    int targetCols = m_columns;
    if (targetCols <= 0) {
        targetCols = (m_rows > 0) ? std::max(1, static_cast<int>((visibleChildren.size() + m_rows - 1) / m_rows)) : 2;
    }

    std::vector<GridItemInfo> itemsInfo;
    std::set<std::pair<int, int>> occupiedCells;

    int autoRow = 0;
    int autoCol = 0;

    for (Item *item : visibleChildren) {
        auto *attached = qobject_cast<LayoutAttached *>(qmlAttachedPropertiesObject<Layout>(item, true));
        int r = attached ? attached->row : -1;
        int c = attached ? attached->column : -1;
        int rSpan = std::max(1, attached ? attached->rowSpan : 1);
        int cSpan = std::max(1, attached ? attached->columnSpan : 1);

        if (r < 0 || c < 0) {
            while (occupiedCells.count({autoRow, autoCol})) {
                if (m_flow == LeftToRight) {
                    autoCol++;
                    if (targetCols > 0 && autoCol >= targetCols) {
                        autoCol = 0;
                        autoRow++;
                    }
                } else {
                    autoRow++;
                    if (m_rows > 0 && autoRow >= m_rows) {
                        autoRow = 0;
                        autoCol++;
                    }
                }
            }
            r = autoRow;
            c = autoCol;
        }

        for (int dr = 0; dr < rSpan; ++dr) {
            for (int dc = 0; dc < cSpan; ++dc) {
                occupiedCells.insert({r + dr, c + dc});
            }
        }

        itemsInfo.push_back({item, r, c, rSpan, cSpan, attached});

        if (m_flow == LeftToRight) {
            autoCol = c + cSpan;
            autoRow = r;
            if (targetCols > 0 && autoCol >= targetCols) {
                autoCol = 0;
                autoRow++;
            }
        } else {
            autoRow = r + rSpan;
            autoCol = c;
            if (m_rows > 0 && autoRow >= m_rows) {
                autoRow = 0;
                autoCol++;
            }
        }
    }

    int maxRow = 0;
    int maxCol = 0;
    for (const auto &info : itemsInfo) {
        maxRow = std::max(maxRow, info.row + info.rowSpan);
        maxCol = std::max(maxCol, info.col + info.colSpan);
    }

    std::vector<int> colWidths(maxCol, 0);
    std::vector<int> rowHeights(maxRow, 0);
    std::vector<bool> colFill(maxCol, false);
    std::vector<bool> rowFill(maxRow, false);

    for (const auto &info : itemsInfo) {
        int prefW = info.item->implicitWidth();
        int prefH = info.item->implicitHeight();
        if (info.layout) {
            if (info.layout->preferredWidth >= 0) prefW = info.layout->preferredWidth;
            if (info.layout->preferredHeight >= 0) prefH = info.layout->preferredHeight;
            if (info.layout->fillWidth) colFill[info.col] = true;
            if (info.layout->fillHeight) rowFill[info.row] = true;
        }

        if (info.colSpan == 1) {
            colWidths[info.col] = std::max(colWidths[info.col], prefW);
        }
        if (info.rowSpan == 1) {
            rowHeights[info.row] = std::max(rowHeights[info.row], prefH);
        }
    }

    for (const auto &info : itemsInfo) {
        int prefW = info.item->implicitWidth();
        int prefH = info.item->implicitHeight();
        if (info.layout) {
            if (info.layout->preferredWidth >= 0) prefW = info.layout->preferredWidth;
            if (info.layout->preferredHeight >= 0) prefH = info.layout->preferredHeight;
        }

        if (info.colSpan > 1) {
            int current = 0;
            for (int c = info.col; c < info.col + info.colSpan; ++c) current += colWidths[c];
            if (prefW > current) {
                int add = (prefW - current) / info.colSpan;
                for (int c = info.col; c < info.col + info.colSpan; ++c) colWidths[c] += add;
            }
        }
        if (info.rowSpan > 1) {
            int current = 0;
            for (int r = info.row; r < info.row + info.rowSpan; ++r) current += rowHeights[r];
            if (prefH > current) {
                int add = (prefH - current) / info.rowSpan;
                for (int r = info.row; r < info.row + info.rowSpan; ++r) rowHeights[r] += add;
            }
        }
    }

    const int totalSpacingW = std::max(0, maxCol - 1) * m_columnSpacing;
    int totalColW = totalSpacingW;
    for (int w : colWidths) totalColW += w;

    const int totalSpacingH = std::max(0, maxRow - 1) * m_rowSpacing;
    int totalRowH = totalSpacingH;
    for (int h : rowHeights) totalRowH += h;

    const int availW = m_width > 0 ? std::max(0, m_width - leftPadding() - rightPadding()) : 0;
    if (availW > totalColW) {
        int fillColsCount = 0;
        for (bool fill : colFill) if (fill) fillColsCount++;
        if (fillColsCount > 0) {
            int extraPerCol = (availW - totalColW) / fillColsCount;
            for (int c = 0; c < maxCol; ++c) {
                if (colFill[c]) colWidths[c] += extraPerCol;
            }
        }
    }

    const int availH = m_height > 0 ? std::max(0, m_height - topPadding() - bottomPadding()) : 0;
    if (availH > totalRowH) {
        int fillRowsCount = 0;
        for (bool fill : rowFill) if (fill) fillRowsCount++;
        if (fillRowsCount > 0) {
            int extraPerRow = (availH - totalRowH) / fillRowsCount;
            for (int r = 0; r < maxRow; ++r) {
                if (rowFill[r]) rowHeights[r] += extraPerRow;
            }
        }
    }

    std::vector<int> colX(maxCol, leftPadding());
    for (int c = 1; c < maxCol; ++c) {
        colX[c] = colX[c - 1] + colWidths[c - 1] + m_columnSpacing;
    }

    std::vector<int> rowY(maxRow, topPadding());
    for (int r = 1; r < maxRow; ++r) {
        rowY[r] = rowY[r - 1] + rowHeights[r - 1] + m_rowSpacing;
    }

    for (const auto &info : itemsInfo) {
        int cellX = colX[info.col];
        int cellY = rowY[info.row];

        int cellW = 0;
        for (int c = info.col; c < info.col + info.colSpan; ++c) cellW += colWidths[c];
        cellW += (info.colSpan - 1) * m_columnSpacing;

        int cellH = 0;
        for (int r = info.row; r < info.row + info.rowSpan; ++r) cellH += rowHeights[r];
        cellH += (info.rowSpan - 1) * m_rowSpacing;

        int itemW = info.item->implicitWidth();
        int itemH = info.item->implicitHeight();

        bool fillW = false;
        bool fillH = false;
        Qt::Alignment align = Qt::AlignLeft | Qt::AlignTop;

        if (info.layout) {
            if (info.layout->preferredWidth >= 0) itemW = info.layout->preferredWidth;
            if (info.layout->preferredHeight >= 0) itemH = info.layout->preferredHeight;
            fillW = info.layout->fillWidth;
            fillH = info.layout->fillHeight;
            align = info.layout->alignment;
        }

        if (fillW) itemW = cellW;
        else itemW = std::min(itemW, cellW);

        if (fillH) itemH = cellH;
        else itemH = std::min(itemH, cellH);

        int posX = cellX;
        if (!fillW) {
            if (align & Qt::AlignRight) posX = cellX + cellW - itemW;
            else if (align & Qt::AlignHCenter) posX = cellX + (cellW - itemW) / 2;
        }

        int posY = cellY;
        if (!fillH) {
            if (align & Qt::AlignBottom) posY = cellY + cellH - itemH;
            else if (align & Qt::AlignVCenter) posY = cellY + (cellH - itemH) / 2;
        }

        info.item->setX(posX);
        info.item->setY(posY);
        info.item->setWidth(itemW);
        info.item->setHeight(itemH);
    }
    clearDirty(DirtyFlag::LayoutDirty);
}

int GridLayout::implicitWidth() const {
    const auto childItems = children();
    int targetCols = m_columns;
    int childCount = 0;
    for (QObject *child : childItems) {
        if (auto *item = qobject_cast<Item *>(child)) {
            if (item->visible()) childCount++;
        }
    }
    if (childCount == 0) return leftPadding() + rightPadding();
    if (targetCols <= 0) targetCols = 2;

    std::vector<int> colWidths(targetCols, 0);
    int colIndex = 0;
    for (QObject *child : childItems) {
        if (auto *item = qobject_cast<Item *>(child)) {
            if (!item->visible()) continue;
            auto *attached = qobject_cast<LayoutAttached *>(qmlAttachedPropertiesObject<Layout>(item, false));
            int prefW = item->implicitWidth();
            if (attached && attached->preferredWidth >= 0) prefW = attached->preferredWidth;
            int col = (attached && attached->column >= 0) ? attached->column : (colIndex % targetCols);
            if (col < targetCols) {
                colWidths[col] = std::max(colWidths[col], prefW);
            }
            colIndex++;
        }
    }

    int totalW = leftPadding() + rightPadding() + std::max(0, targetCols - 1) * m_columnSpacing;
    for (int w : colWidths) totalW += w;
    return totalW;
}

int GridLayout::implicitHeight() const {
    const auto childItems = children();
    int targetCols = m_columns;
    int childCount = 0;
    for (QObject *child : childItems) {
        if (auto *item = qobject_cast<Item *>(child)) {
            if (item->visible()) childCount++;
        }
    }
    if (childCount == 0) return topPadding() + bottomPadding();
    if (targetCols <= 0) targetCols = 2;

    int totalRows = (childCount + targetCols - 1) / targetCols;
    std::vector<int> rowHeights(totalRows, 0);
    int itemIndex = 0;
    for (QObject *child : childItems) {
        if (auto *item = qobject_cast<Item *>(child)) {
            if (!item->visible()) continue;
            auto *attached = qobject_cast<LayoutAttached *>(qmlAttachedPropertiesObject<Layout>(item, false));
            int prefH = item->implicitHeight();
            if (attached && attached->preferredHeight >= 0) prefH = attached->preferredHeight;
            int r = (attached && attached->row >= 0) ? attached->row : (itemIndex / targetCols);
            if (r < totalRows) {
                rowHeights[r] = std::max(rowHeights[r], prefH);
            }
            itemIndex++;
        }
    }

    int totalH = topPadding() + bottomPadding() + std::max(0, totalRows - 1) * m_rowSpacing;
    for (int h : rowHeights) totalH += h;
    return totalH;
}

void GridLayout::updateLayout() {
    layoutChildren();
}

} // namespace qtui
