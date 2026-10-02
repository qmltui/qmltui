#include "FlexLayout.h"
#include <numeric>
#include <algorithm>
#include <vector>

namespace qtui {

struct FlexItemInfo {
    Item* item;
    int baseMainSize = 0;
    int baseCrossSize = 0;
    int currentMainSize = 0;
    int currentCrossSize = 0;
};

struct FlexLine {
    std::vector<FlexItemInfo> items;
    int mainSizeSum = 0;
    int maxCrossSize = 0;
};

FlexLayout::FlexLayout(QObject *parent) : Item(parent) {
}

FlexLayout::~FlexLayout() = default;

void FlexLayout::setFlexDirection(FlexDirection dir) {
    if (m_flexDirection == dir) return;
    m_flexDirection = dir;
    emit flexDirectionChanged();
    markDirty();
}

void FlexLayout::setFlexWrap(FlexWrap wrap) {
    if (m_flexWrap == wrap) return;
    m_flexWrap = wrap;
    emit flexWrapChanged();
    markDirty();
}

void FlexLayout::setJustifyContent(JustifyContent justify) {
    if (m_justifyContent == justify) return;
    m_justifyContent = justify;
    emit justifyContentChanged();
    markDirty();
}

void FlexLayout::setAlignItems(AlignItems align) {
    if (m_alignItems == align) return;
    m_alignItems = align;
    emit alignItemsChanged();
    markDirty();
}

void FlexLayout::doLayout() {
    bool isRow = (m_flexDirection == Row || m_flexDirection == RowReverse);
    bool isReverse = (m_flexDirection == RowReverse || m_flexDirection == ColumnReverse);

    int availMain = isRow ? width() : height();
    int availCross = isRow ? height() : width();

    // 1. Gather visible direct children
    std::vector<Item*> children;
    for (Item* child : childItems()) {
        if (child && child->visible()) {
            children.push_back(child);
        }
    }
    if (children.empty()) return;

    // 2. Wrap items into lines
    std::vector<FlexLine> lines;
    FlexLine currentLine;

    for (Item* child : children) {
        FlexItemInfo info;
        info.item = child;

        // Base main size
        int explicitMain = isRow ? child->width() : child->height();
        if (explicitMain > 0) {
            info.baseMainSize = explicitMain;
        } else if (child->flexBasis() >= 0) {
            info.baseMainSize = child->flexBasis();
        } else {
            info.baseMainSize = isRow ? child->implicitWidth() : child->implicitHeight();
        }

        // Base cross size
        int explicitCross = isRow ? child->height() : child->width();
        if (explicitCross > 0) {
            info.baseCrossSize = explicitCross;
        } else {
            info.baseCrossSize = isRow ? child->implicitHeight() : child->implicitWidth();
        }

        // If wrapping is enabled, check if we need to start a new line
        if (m_flexWrap == Wrap && !currentLine.items.empty() && currentLine.mainSizeSum + info.baseMainSize > availMain) {
            lines.push_back(currentLine);
            currentLine = FlexLine();
        }

        currentLine.items.push_back(info);
        currentLine.mainSizeSum += info.baseMainSize;
        currentLine.maxCrossSize = std::max(currentLine.maxCrossSize, info.baseCrossSize);
    }
    if (!currentLine.items.empty()) {
        lines.push_back(currentLine);
    }

    // If wrap is disabled and we have only one line, set its maxCrossSize to availCross if alignment is Stretch
    if (m_flexWrap == NoWrap && !lines.empty() && m_alignItems == AlignStretch) {
        lines[0].maxCrossSize = std::max(lines[0].maxCrossSize, availCross);
    }

    // 3. Size and position items inside each line
    int crossOffset = 0;
    for (size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        FlexLine& line = lines[lineIdx];
        int remaining = availMain - line.mainSizeSum;

        double growSum = 0.0;
        double shrinkSum = 0.0;
        for (const auto& info : line.items) {
            growSum += info.item->flexGrow();
            shrinkSum += info.item->flexShrink() * info.baseMainSize;
        }

        // Main axis sizing distribution
        if (remaining > 0 && growSum > 0.0) {
            for (auto& info : line.items) {
                double grow = info.item->flexGrow();
                info.currentMainSize = info.baseMainSize + static_cast<int>((grow / growSum) * remaining);
            }
        } else if (remaining < 0 && shrinkSum > 0.0) {
            for (auto& info : line.items) {
                double shrink = info.item->flexShrink();
                double weight = shrink * info.baseMainSize;
                info.currentMainSize = info.baseMainSize + static_cast<int>((weight / shrinkSum) * remaining);
                info.currentMainSize = std::max(0, info.currentMainSize);
            }
        } else {
            for (auto& info : line.items) {
                info.currentMainSize = info.baseMainSize;
            }
        }

        // Recompute main size sum
        int finalMainSizeSum = 0;
        for (const auto& info : line.items) {
            finalMainSizeSum += info.currentMainSize;
        }
        int finalRemaining = availMain - finalMainSizeSum;

        // Justify content alignment values
        int mainStart = 0;
        int mainGap = 0;
        int itemCount = static_cast<int>(line.items.size());

        switch (m_justifyContent) {
            case FlexStart:
                mainStart = 0;
                mainGap = 0;
                break;
            case FlexEnd:
                mainStart = finalRemaining;
                mainGap = 0;
                break;
            case Center:
                mainStart = finalRemaining / 2;
                mainGap = 0;
                break;
            case SpaceBetween:
                mainStart = 0;
                mainGap = itemCount > 1 ? finalRemaining / (itemCount - 1) : 0;
                break;
            case SpaceAround:
                mainGap = itemCount > 0 ? finalRemaining / itemCount : 0;
                mainStart = mainGap / 2;
                break;
            case SpaceEvenly:
                mainGap = itemCount > 0 ? finalRemaining / (itemCount + 1) : 0;
                mainStart = mainGap;
                break;
        }

        // Apply main/cross position & size to items
        int currentMain = mainStart;
        
        std::vector<FlexItemInfo> orderedItems = line.items;
        if (isReverse) {
            std::reverse(orderedItems.begin(), orderedItems.end());
        }

        for (auto& info : orderedItems) {
            // Cross axis alignment (AlignItems)
            int itemCrossSize = info.baseCrossSize;
            int itemCrossPos = 0;

            switch (m_alignItems) {
                case AlignStart:
                    itemCrossPos = 0;
                    break;
                case AlignEnd:
                    itemCrossPos = line.maxCrossSize - itemCrossSize;
                    break;
                case AlignCenter:
                    itemCrossPos = (line.maxCrossSize - itemCrossSize) / 2;
                    break;
                case AlignStretch:
                    itemCrossPos = 0;
                    // If no explicit cross size or stretch is forced, stretch it to max cross size
                    if (isRow ? (info.item->height() <= 0) : (info.item->width() <= 0)) {
                        itemCrossSize = line.maxCrossSize;
                    }
                    break;
            }

            int childX = 0;
            int childY = 0;
            int childW = 0;
            int childH = 0;

            if (isRow) {
                childX = currentMain;
                childY = crossOffset + itemCrossPos;
                childW = info.currentMainSize;
                childH = itemCrossSize;
            } else {
                childX = crossOffset + itemCrossPos;
                childY = currentMain;
                childW = itemCrossSize;
                childH = info.currentMainSize;
            }

            info.item->setGeometryDirect(childX, childY, childW, childH);
            currentMain += info.currentMainSize + mainGap;
        }

        crossOffset += line.maxCrossSize;
    }
}

void FlexLayout::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    // 1. Position and resize direct children first
    doLayout();
    
    // 2. Perform standard rendering
    Item::render(buffer, absX, absY, clipRect);
}

} // namespace qtui
