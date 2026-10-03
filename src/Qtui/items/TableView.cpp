#include "TableView.h"
#include "DelegateUtils.h"
#include <ColorMapper.h>
#include "KeyEvent.h"
#include "MouseEvent.h"
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlProperty>
#include <algorithm>
#include <UnicodeText.h>

namespace qtui {

static QQmlContext *delegateContext(Item *item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly)
                : nullptr;
}

static void setPropertyIfExists(QObject* obj, const char* name, const QVariant& value) {
    if (!obj) return;
    if (obj->metaObject()->indexOfProperty(name) != -1) {
        obj->setProperty(name, value);
    }
}

static QString alignText(const QString &text, int width, const QString &alignment) {
    if (width <= 0) return QString();
    QString t;
    int usedWidth = 0;
    for (const Grapheme &cluster : UnicodeText::graphemes(text)) {
        if (usedWidth + cluster.width > width) break;
        t += QString::fromUtf8(cluster.utf8);
        usedWidth += cluster.width;
    }
    const int pad = width - usedWidth;
    if (alignment == "right") {
        return QString(pad, ' ') + t;
    } else if (alignment == "center") {
        int leftPad = pad / 2;
        return QString(leftPad, ' ') + t + QString(pad - leftPad, ' ');
    }
    return t + QString(pad, ' ');
}

static bool compareVariants(const QVariant &a, const QVariant &b, bool ascending) {
    bool okA = false, okB = false;
    double dA = a.toDouble(&okA);
    double dB = b.toDouble(&okB);
    if (okA && okB) {
        return ascending ? (dA < dB) : (dA > dB);
    }
    return ascending ? (a.toString() < b.toString()) : (a.toString() > b.toString());
}

TableView::TableView(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(40);
    setImplicitHeight(10);
}

TableView::~TableView() {
    clearDelegateItems();
}

void TableView::clearDelegateItems() {
    for (Item* item : m_createdDelegateItems.values()) {
        if (item) {
            item->setParentItem(nullptr);
            delete item;
        }
    }
    m_createdDelegateItems.clear();
}

void TableView::setColumns(const QVariantList &columns) {
    m_columns = columns;
    emit columnsChanged();
    generateDelegateItems();
    markDirty();
}

void TableView::setModel(const QVariant &model) {
    if (m_cppModel)
        disconnect(m_cppModel.data(), nullptr, this, nullptr);
    m_modelVariant = model;
    m_listModel.clear();
    m_cppModel = nullptr;

    if (model.userType() == QMetaType::fromType<QJSValue>().id()) {
        QJSValue jsVal = model.value<QJSValue>();
        m_listModel = jsVal.toVariant().toList();
    } else if (model.canConvert<QVariantList>()) {
        m_listModel = model.toList();
    } else if (QObject *obj = model.value<QObject*>()) {
        m_cppModel = qobject_cast<QAbstractItemModel*>(obj);
    }

    if (m_cppModel) {
        connect(m_cppModel.data(), &QObject::destroyed, this, [this]() {
            m_cppModel.clear();
            m_modelVariant.clear();
            clearDelegateItems();
            emit modelChanged();
            markDirty();
        });
        connect(m_cppModel.data(), &QAbstractItemModel::dataChanged, this,
                [this]() { generateDelegateItems(); markDirty(); });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsInserted,
                this, [this]() { generateDelegateItems(); markDirty(); });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsRemoved,
                this, [this]() { generateDelegateItems(); markDirty(); });
        connect(m_cppModel.data(), &QAbstractItemModel::rowsMoved,
                this, [this]() { generateDelegateItems(); markDirty(); });
        connect(m_cppModel.data(), &QAbstractItemModel::modelReset,
                this, [this]() { generateDelegateItems(); markDirty(); });
        connect(m_cppModel.data(), &QAbstractItemModel::layoutChanged,
                this, [this]() { generateDelegateItems(); markDirty(); });
    }

    if (m_sortColumn >= 0) {
        sortModel();
    }

    emit modelChanged();
    generateDelegateItems();
    markDirty();
}

void TableView::setSelectedRow(int row) {
    const int maxR = rowCount() - 1;
    const int clamped = row < 0 || maxR < 0 ? -1 : (std::min)(row, maxR);
    if (m_selectedRow == clamped) return;
    m_selectedRow = clamped;
    emit selectedRowChanged();
    if (m_selectedRow >= 0 && m_currentColumn < 0) {
        m_currentColumn = 0;
        emit currentColumnChanged();
    }
    if (m_delegate || m_rowDelegate) generateDelegateItems();
    markDirty();
}

void TableView::setCurrentColumn(int col) {
    const int maxColumn = (m_columns.isEmpty() ? 5 : m_columns.size()) - 1;
    const int normalized = col < 0 || maxColumn < 0 ? -1 : (std::min)(col, maxColumn);
    if (m_currentColumn == normalized) return;
    m_currentColumn = normalized;
    emit currentColumnChanged();
    if (m_delegate || m_rowDelegate) generateDelegateItems();
    markDirty();
}

void TableView::setAlternatingRows(bool alt) {
    if (m_alternatingRows == alt) return;
    m_alternatingRows = alt;
    emit alternatingRowsChanged();
    markDirty();
}

void TableView::setSortColumn(int col) {
    if (m_sortColumn == col) return;
    m_sortColumn = col;
    emit sortColumnChanged();
    sortModel();
}

void TableView::setSortOrder(Qt::SortOrder order) {
    if (m_sortOrder == order) return;
    m_sortOrder = order;
    emit sortOrderChanged();
    sortModel();
}

void TableView::sortByColumn(int column, Qt::SortOrder order) {
    m_sortColumn = column;
    m_sortOrder = order;
    emit sortColumnChanged();
    emit sortOrderChanged();
    sortModel();
}

void TableView::sortModel() {
    if (m_sortColumn < 0) return;

    if (m_cppModel) {
        m_cppModel->sort(m_sortColumn, m_sortOrder);
    } else if (!m_listModel.isEmpty()) {
        int col = m_sortColumn;
        bool asc = (m_sortOrder == Qt::AscendingOrder);

        std::stable_sort(m_listModel.begin(), m_listModel.end(), [this, col, asc](const QVariant &rowA, const QVariant &rowB) {
            QVariant valA, valB;
            
            QVariant rA = rowA;
            if (rA.userType() == QMetaType::fromType<QJSValue>().id()) rA = rA.value<QJSValue>().toVariant();
            if (rA.canConvert<QVariantList>()) {
                QVariantList l = rA.toList();
                if (col >= 0 && col < l.size()) valA = l[col];
            } else if (rA.canConvert<QVariantMap>()) {
                QVariantMap m = rA.toMap();
                if (col >= 0 && col < m_columns.size()) {
                    QVariant colVal = m_columns[col];
                    QString key = colVal.canConvert<QVariantMap>() ? colVal.toMap().value("key", colVal.toMap().value("title")).toString() : colVal.toString();
                    valA = m.value(key);
                }
            }

            QVariant rB = rowB;
            if (rB.userType() == QMetaType::fromType<QJSValue>().id()) rB = rB.value<QJSValue>().toVariant();
            if (rB.canConvert<QVariantList>()) {
                QVariantList l = rB.toList();
                if (col >= 0 && col < l.size()) valB = l[col];
            } else if (rB.canConvert<QVariantMap>()) {
                QVariantMap m = rB.toMap();
                if (col >= 0 && col < m_columns.size()) {
                    QVariant colVal = m_columns[col];
                    QString key = colVal.canConvert<QVariantMap>() ? colVal.toMap().value("key", colVal.toMap().value("title")).toString() : colVal.toString();
                    valB = m.value(key);
                }
            }

            return compareVariants(valA, valB, asc);
        });
    }

    if (!m_cppModel && m_delegate)
        generateDelegateItems();
    markDirty();
}

void TableView::setHeaderColor(const QString &color) {
    if (m_headerColor == color) return;
    m_headerColor = color;
    m_resolvedHeaderColor = ColorMapper::fromString(m_headerColor, 0xFFFFFF00);
    emit headerColorChanged();
    markDirty();
}

void TableView::setHeaderBgColor(const QString &color) {
    if (m_headerBgColor == color) return;
    m_headerBgColor = color;
    m_resolvedHeaderBgColor = ColorMapper::fromString(m_headerBgColor, 0x00000000);
    emit headerBgColorChanged();
    markDirty();
}

void TableView::setSelectionColor(const QString &color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0xFFFFFFFF);
    emit selectionColorChanged();
    markDirty();
}

void TableView::setSelectionBgColor(const QString &color) {
    if (m_selectionBgColor == color) return;
    m_selectionBgColor = color;
    m_resolvedSelectionBgColor = ColorMapper::fromString(m_selectionBgColor, 0xFF0000FF);
    emit selectionBgColorChanged();
    markDirty();
}

void TableView::setTextColor(const QString &color) {
    if (m_textColor == color) return;
    m_textColor = color;
    m_resolvedTextColor = ColorMapper::fromString(m_textColor, 0xFFCCCCCC);
    emit textColorChanged();
    markDirty();
}

void TableView::setRowBgColor(const QString &color) {
    if (m_rowBgColor == color) return;
    m_rowBgColor = color;
    m_resolvedRowBgColor = ColorMapper::fromString(m_rowBgColor, 0x00000000);
    emit rowBgColorChanged();
    markDirty();
}

void TableView::setAlternatingRowBgColor(const QString &color) {
    if (m_alternatingRowBgColor == color) return;
    m_alternatingRowBgColor = color;
    m_resolvedAltRowBgColor = ColorMapper::fromString(m_alternatingRowBgColor, 0xFF1E1E2E);
    emit alternatingRowBgColorChanged();
    markDirty();
}

void TableView::setShowGrid(bool show) {
    if (m_showGrid == show) return;
    m_showGrid = show;
    emit showGridChanged();
    markDirty();
}

void TableView::setBorderStyle(const QString &style) {
    if (m_borderStyle == style) return;
    m_borderStyle = style;
    emit borderStyleChanged();
    markDirty();
}

void TableView::setBorderColor(const QString &color) {
    if (m_borderColor == color) return;
    m_borderColor = color;
    m_resolvedBorderColor = ColorMapper::fromString(m_borderColor, 0xFF808080);
    emit borderColorChanged();
    markDirty();
}

void TableView::setColumnWidthProvider(const QJSValue &provider) {
    m_columnWidthProvider = provider;
    emit columnWidthProviderChanged();
    markDirty();
}

void TableView::setCellFormat(const QJSValue &format) {
    m_cellFormat = format;
    emit cellFormatChanged();
    markDirty();
}

void TableView::setDelegate(QQmlComponent *delegate) {
    if (m_delegate == delegate) return;
    clearDelegateItems();
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            clearDelegateItems();
            emit delegateChanged();
            markDirty();
        });
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
            if (status == QQmlComponent::Ready)
                generateDelegateItems();
            else if (status == QQmlComponent::Error)
                DelegateUtils::isReady(m_delegate, this);
        });
    }
    if (m_delegate && m_rowDelegate)
        qmlWarning(this) << "TableView.delegate overrides deprecated rowDelegate";
    emit delegateChanged();
    generateDelegateItems();
    markDirty();
}

void TableView::setRowDelegate(QQmlComponent *delegate)
{
    if (m_rowDelegate == delegate) return;
    clearDelegateItems();
    if (m_rowDelegate) disconnect(m_rowDelegate, nullptr, this, nullptr);
    m_rowDelegate = delegate;
    if (m_rowDelegate) {
        connect(m_rowDelegate, &QObject::destroyed, this, [this]() {
            m_rowDelegate.clear();
            clearDelegateItems();
            emit rowDelegateChanged();
            markDirty();
        });
        connect(m_rowDelegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
            if (status == QQmlComponent::Ready && !m_delegate)
                generateDelegateItems();
            else if (status == QQmlComponent::Error)
                DelegateUtils::isReady(m_rowDelegate, this);
        });
    }
    if (m_delegate && m_rowDelegate)
        qmlWarning(this) << "TableView.delegate overrides deprecated rowDelegate";
    emit rowDelegateChanged();
    generateDelegateItems();
    markDirty();
}

int TableView::rowCount() const {
    if (m_cppModel) {
        return m_cppModel->rowCount();
    }
    return static_cast<int>(m_listModel.size());
}

int TableView::columnWidth(int col) const {
    if (m_columnWidthProvider.isCallable()) {
        QJSValue res = const_cast<QJSValue&>(m_columnWidthProvider).call(QJSValueList{ col });
        if (res.isNumber()) return res.toInt();
    }
    if (col >= 0 && col < m_columns.size()) {
        QVariant colVal = m_columns[col];
        if (colVal.userType() == QMetaType::fromType<QJSValue>().id()) {
            colVal = colVal.value<QJSValue>().toVariant();
        }
        if (colVal.canConvert<QVariantMap>()) {
            return colVal.toMap().value("width", 12).toInt();
        }
    }
    return 12;
}

QString TableView::columnAlignment(int col) const {
    if (col >= 0 && col < m_columns.size()) {
        QVariant colVal = m_columns[col];
        if (colVal.userType() == QMetaType::fromType<QJSValue>().id()) {
            colVal = colVal.value<QJSValue>().toVariant();
        }
        if (colVal.canConvert<QVariantMap>()) {
            return colVal.toMap().value("align", "left").toString();
        }
    }
    return "left";
}

QString TableView::cellText(int row, int col) const {
    if (m_cppModel) {
        QModelIndex idx = m_cppModel->index(row, col);
        return m_cppModel->data(idx, Qt::DisplayRole).toString();
    }
    if (row >= 0 && row < m_listModel.size()) {
        QVariant r = m_listModel[row];
        if (r.userType() == QMetaType::fromType<QJSValue>().id()) {
            r = r.value<QJSValue>().toVariant();
        }
        if (r.canConvert<QVariantList>()) {
            QVariantList rowList = r.toList();
            if (col >= 0 && col < rowList.size()) return rowList[col].toString();
        } else if (r.canConvert<QVariantMap>()) {
            QVariantMap rowMap = r.toMap();
            if (col >= 0 && col < m_columns.size()) {
                QVariant colVal = m_columns[col];
                QString key;
                if (colVal.canConvert<QVariantMap>()) {
                    QVariantMap colMap = colVal.toMap();
                    key = colMap.value("key", colMap.value("title")).toString();
                } else {
                    key = colVal.toString();
                }
                return rowMap.value(key).toString();
            }
        }
    }
    return QString();
}



Item* TableView::createSingleDelegateItem(int row, int column)
{
    QQmlComponent *component = m_delegate ? m_delegate.data() : m_rowDelegate.data();
    if (!DelegateUtils::isReady(component, this)) return nullptr;

    const bool cellMode = m_delegate;
    const int delegateColumns = cellMode ? (m_columns.isEmpty() ? 5 : m_columns.size()) : 1;
    const int itemIndex = cellMode ? row * delegateColumns + column : row;

    QVariantMap modelMap;
    QVariant modelData;

    const QHash<int, QByteArray> roles = m_cppModel
        ? m_cppModel->roleNames() : QHash<int, QByteArray>();

    if (m_cppModel) {
        const QModelIndex index = m_cppModel->index(row, cellMode ? column : 0);
        for (auto role = roles.cbegin(); role != roles.cend(); ++role)
            modelMap.insert(QString::fromUtf8(role.value()),
                            m_cppModel->data(index, role.key()));
        modelData = modelMap.isEmpty()
            ? m_cppModel->data(index, Qt::DisplayRole) : QVariant(modelMap);
    } else if (cellMode) {
        modelData = cellText(row, column);
        if (row < m_listModel.size()) {
            QVariant rowValue = m_listModel[row];
            if (rowValue.userType() == QMetaType::fromType<QJSValue>().id())
                rowValue = rowValue.value<QJSValue>().toVariant();
            if (rowValue.canConvert<QVariantMap>())
                modelMap = rowValue.toMap();
        }
        modelMap.insert(QStringLiteral("display"), modelData);
    } else if (row < m_listModel.size()) {
        modelData = m_listModel[row];
        QVariant value = modelData;
        if (value.userType() == QMetaType::fromType<QJSValue>().id())
            value = value.value<QJSValue>().toVariant();
        if (value.canConvert<QVariantMap>()) modelMap = value.toMap();
    }

    const bool current = row == m_selectedRow && column == m_currentColumn;
    QVariantMap initialProperties = modelMap;
    initialProperties[QStringLiteral("index")] = itemIndex;
    initialProperties[QStringLiteral("row")] = row;
    if (cellMode) initialProperties[QStringLiteral("column")] = column;
    initialProperties[QStringLiteral("current")] = current;
    initialProperties[QStringLiteral("selected")] = current;
    initialProperties[QStringLiteral("model")] = modelMap;
    initialProperties[QStringLiteral("modelData")] = modelData;

    QQmlContext *context = DelegateUtils::createContext(component, this);
    if (!context) return nullptr;

    for (auto it = initialProperties.cbegin(); it != initialProperties.cend(); ++it)
        DelegateUtils::setContextProperty(component, context, it.key(), it.value());

    Item *item = DelegateUtils::createItem(
        component, initialProperties, context, this, this);
    if (item) {
        item->setWidth(cellMode ? columnWidth(column) : width());
        if (item->height() <= 0) item->setHeight(1);
    }
    return item;
}

void TableView::updateDelegateViewport(int firstRow, int lastRow, int firstCol, int lastCol)
{
    if (!m_delegate && !m_rowDelegate) {
        clearDelegateItems();
        return;
    }

    // 1. Remove out-of-bounds items
    for (auto it = m_createdDelegateItems.begin(); it != m_createdDelegateItems.end();) {
        int r = it.key().first;
        int c = it.key().second;
        if (r < firstRow || r > lastRow || c < firstCol || c > lastCol) {
            Item *item = it.value();
            if (item) {
                item->setParentItem(nullptr);
                delete item;
            }
            it = m_createdDelegateItems.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Instantiate missing visible items
    for (int r = firstRow; r <= lastRow; ++r) {
        for (int c = firstCol; c <= lastCol; ++c) {
            QPair<int, int> pos(r, c);
            if (!m_createdDelegateItems.contains(pos)) {
                Item *item = createSingleDelegateItem(r, c);
                if (item) {
                    m_createdDelegateItems.insert(pos, item);
                }
            }
        }
    }
}

void TableView::generateDelegateItems()
{
    clearDelegateItems();
    markDirty();
}

void TableView::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible) return;

    Item::render(buffer, absX, absY, clipRect);

    int availW = width();
    int availH = height();
    int startX = absX;
    int startY = absY;

    if (m_borderStyle != "none") {
        buffer.drawBox(absX, absY, availW, availH, m_resolvedBorderColor, 0x00000000);
        startX += 1;
        startY += 1;
        availW -= 2;
        availH -= 2;
    }

    if (availW <= 0 || availH <= 0) return;

    bool hasHeaders = (!m_columns.isEmpty() || m_columnWidthProvider.isCallable());

    // 1. Render Header Row
    if (hasHeaders) {
        int curX = startX;
        int numCols = m_columns.isEmpty() ? 5 : m_columns.size();

        for (int col = 0; col < numCols; ++col) {
            if (curX >= startX + availW) break; // Horizontal clipping

            QString title = QString("Col%1").arg(col + 1);
            int colW = columnWidth(col);
            QString align = columnAlignment(col);

            if (col < m_columns.size()) {
                QVariant colVal = m_columns[col];
                if (colVal.userType() == QMetaType::fromType<QJSValue>().id()) {
                    colVal = colVal.value<QJSValue>().toVariant();
                }
                if (colVal.canConvert<QVariantMap>()) {
                    title = colVal.toMap().value("title", title).toString();
                } else {
                    title = colVal.toString();
                }
            }

            // Append sort indicator glyph if this column is sorted
            if (m_sortColumn == col) {
                title += (m_sortOrder == Qt::AscendingOrder ? " ▲" : " ▼");
            }

            QString formattedHeader = alignText(title, colW, align);
            buffer.writeText(curX, startY, formattedHeader, m_resolvedHeaderColor, m_resolvedHeaderBgColor, TextAttribute::Bold);
            curX += colW;

            if (m_showGrid && col < numCols - 1 && curX < startX + availW) {
                buffer.cellAt(curX - 1, startY).character = "|";
            }
        }

        // Header divider line
        buffer.drawLine(startX, startY + 1, startX + availW - 1, startY + 1, "─", m_resolvedHeaderColor);
        startY += 2;
        availH -= 2;
    }

    // 2. Compute Viewport Vertical Scroll Offset
    int totalRows = rowCount();
    int visibleRows = (std::max)(1, availH);

    int firstVisibleRow = 0;
    if (m_selectedRow >= visibleRows) {
        firstVisibleRow = m_selectedRow - visibleRows + 1;
    }
    firstVisibleRow = std::clamp(firstVisibleRow, 0, (std::max)(0, totalRows - visibleRows));

    // 3. Render Delegate Component Items
    if (m_delegate || m_rowDelegate) {
        const bool cellMode = m_delegate;
        const int delegateColumns = cellMode
            ? (m_columns.isEmpty() ? 5 : m_columns.size()) : 1;

        int firstRow = (std::max)(0, firstVisibleRow - 2);
        int lastRow = (std::min)(totalRows - 1, firstVisibleRow + visibleRows + 1);

        int firstCol = 0;
        int lastCol = 0;
        int curWidth = 0;
        for (int col = 0; col < delegateColumns; ++col) {
            lastCol = col;
            curWidth += cellMode ? columnWidth(col) : availW;
            if (curWidth >= availW) break;
        }
        if (cellMode && lastCol < delegateColumns - 1) {
            lastCol += 1; // +1 overscan column
        }

        updateDelegateViewport(firstRow, lastRow, firstCol, lastCol);

        for (int visibleRow = 0;
             visibleRow < visibleRows && firstVisibleRow + visibleRow < totalRows;
             ++visibleRow) {
            const int actualRow = firstVisibleRow + visibleRow;
            int cellX = startX;
            for (int column = 0; column <= lastCol; ++column) {
                const int colW = cellMode ? columnWidth(column) : availW;
                if (cellX >= startX + availW) break;

                Item *item = m_createdDelegateItems.value(QPair<int, int>(actualRow, column), nullptr);
                if (item) {
                    item->setWidth(colW);
                    item->render(buffer, cellX, startY + visibleRow, clipRect);
                }
                if (cellMode) cellX += colW;
            }
        }
        return;
    }

    // 4. Render Standard Row Cells
    int numCols = m_columns.isEmpty() ? 5 : m_columns.size();

    QVector<int> colWidths(numCols);
    QVector<QString> colAligns(numCols);
    for (int col = 0; col < numCols; ++col) {
        colWidths[col] = columnWidth(col);
        colAligns[col] = columnAlignment(col);
    }

    for (int r = 0; r < visibleRows && (firstVisibleRow + r) < totalRows; ++r) {
        int actualRow = firstVisibleRow + r;
        int rowY = startY + r;
        bool isSelected = (actualRow == m_selectedRow);

        uint32_t bg = isSelected ? m_resolvedSelectionBgColor :
                      (m_alternatingRows && (actualRow % 2 == 1) ? m_resolvedAltRowBgColor : m_resolvedRowBgColor);
        uint32_t fg = isSelected ? m_resolvedSelectionColor : m_resolvedTextColor;
        TextAttribute attr = isSelected ? TextAttribute::Bold : TextAttribute::None;

        if (isSelected || bg != 0x00000000) {
            buffer.fillRect(startX, rowY, availW, 1, " ", fg, bg);
        }

        int curX = startX;
        for (int col = 0; col < numCols; ++col) {
            if (curX >= startX + availW) break; // Horizontal clipping

            int colW = colWidths[col];
            QString align = colAligns[col];
            QString val = cellText(actualRow, col);

            uint32_t cellFg = fg;
            uint32_t cellBg = bg;
            TextAttribute cellAttr = attr;

            // Apply JS cellFormat callback if defined
            if (m_cellFormat.isCallable()) {
                QJSValue fmtRes = const_cast<QJSValue&>(m_cellFormat).call(QJSValueList{ actualRow, col, val });
                if (fmtRes.isObject()) {
                    if (fmtRes.hasProperty("text")) val = fmtRes.property("text").toString();
                    if (fmtRes.hasProperty("color")) cellFg = ColorMapper::fromString(fmtRes.property("color").toString(), cellFg);
                    if (fmtRes.hasProperty("bgColor")) cellBg = ColorMapper::fromString(fmtRes.property("bgColor").toString(), cellBg);
                    if (fmtRes.hasProperty("bold") && fmtRes.property("bold").toBool()) cellAttr = TextAttribute::Bold;
                    if (fmtRes.hasProperty("align")) align = fmtRes.property("align").toString();
                }
            }

            QString formattedText = alignText(val, colW, align);
            buffer.writeText(curX, rowY, formattedText, cellFg, cellBg, cellAttr);
            curX += colW;

            if (m_showGrid && col < numCols - 1 && curX < startX + availW) {
                buffer.cellAt(curX - 1, rowY).character = "|";
            }
        }
    }
}

void TableView::keyPressEvent(KeyEvent *event) {
    int total = rowCount();
    if (event->key() == Qt::Key_Up) {
        setSelectedRow(m_selectedRow - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Down) {
        setSelectedRow(m_selectedRow + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageUp) {
        setSelectedRow(m_selectedRow - 10);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_PageDown) {
        setSelectedRow(m_selectedRow + 10);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        setCurrentColumn(m_currentColumn + 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Left) {
        setCurrentColumn(m_currentColumn - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Space) {
        emit rowActivated(m_selectedRow);
        event->setAccepted(true);
    }
}

void TableView::mousePressEvent(MouseEvent *event) {
    setFocus(true);
    int borderOffset = (m_borderStyle != "none") ? 1 : 0;
    bool hasHeaders = (!m_columns.isEmpty() || m_columnWidthProvider.isCallable());

    int headerY = borderOffset;
    if (hasHeaders && (event->y() == headerY || event->y() == headerY + 1)) {
        // Clicked header row -> sort by column!
        int curX = borderOffset;
        int numCols = m_columns.isEmpty() ? 5 : m_columns.size();
        for (int col = 0; col < numCols; ++col) {
            int colW = columnWidth(col);
            if (event->x() >= curX && event->x() < curX + colW) {
                Qt::SortOrder newOrder = (m_sortColumn == col && m_sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
                sortByColumn(col, newOrder);
                event->setAccepted(true);
                return;
            }
            curX += colW;
        }
    }

    int offsetY = borderOffset + (hasHeaders ? 2 : 0);
    int clickRow = event->y() - offsetY;

    int totalRows = rowCount();
    int availH = height() - (m_borderStyle != "none" ? 2 : 0) - (hasHeaders ? 2 : 0);
    int visibleRows = (std::max)(1, availH);

    int firstVisibleRow = 0;
    if (m_selectedRow >= visibleRows) {
        firstVisibleRow = m_selectedRow - visibleRows + 1;
    }
    firstVisibleRow = std::clamp(firstVisibleRow, 0, (std::max)(0, totalRows - visibleRows));

    int actualRow = firstVisibleRow + clickRow;

    if (clickRow >= 0 && actualRow < totalRows) {
        setSelectedRow(actualRow);
        emit rowActivated(actualRow);
        event->setAccepted(true);
    }
}

} // namespace qtui
