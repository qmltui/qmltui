#include "TableView.h"
#include "DelegateUtils.h"
#include "KeyEvent.h"
#include "MouseEvent.h"
#include <ColorMapper.h>
#include <UnicodeText.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qtui {
namespace {
QQmlContext *delegateContext(Item *item) {
    return item ? item->findChild<QQmlContext *>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}
QVariant unpack(QVariant value) {
    return value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
}
// Compute alignment against the logical width, but allocate only the visible prefix.
QString alignedPrefix(const QString &text, int width, int visibleWidth, const QString &alignment) {
    if (width <= 0 || visibleWidth <= 0) return {};
    int used = 0;
    GraphemeIterator measure(text);
    GraphemeRef cluster;
    while (measure.next(cluster)) {
        if (cluster.width > width - used) break;
        used += cluster.width;
    }
    const int padding = width - used;
    const int left = alignment == "right" ? padding : alignment == "center" ? padding / 2 : 0;
    QString result((std::min)(left, visibleWidth), QLatin1Char(' '));
    int position = left;
    GraphemeIterator draw(text);
    while (position < visibleWidth && draw.next(cluster)) {
        if (cluster.width > width - position || cluster.width > visibleWidth - position) break;
        result.append(cluster.utf16);
        position += cluster.width;
    }
    if (position < visibleWidth) result.append(QString(visibleWidth - position, QLatin1Char(' ')));
    return result;
}
}

TableView::TableView(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(40);
    setImplicitHeight(10);
    connect(this, &Item::widthChanged, this, &TableView::forceLayout);
    connect(this, &Item::heightChanged, this, &TableView::forceLayout);
    forceLayout();
}
TableView::~TableView() { clearDelegateItems(); }
void TableView::destroyDelegate(DelegateRecord &record) {
    if (record.item) {
        record.item->setParentItem(nullptr);
        delete record.item.data();
    }
    record.item.clear();
    record.component.clear();
}
void TableView::clearDelegateItems() {
    for (auto &record : m_createdDelegateItems) destroyDelegate(record);
    m_createdDelegateItems.clear();
}
void TableView::generateDelegateItems() {
    m_resetDelegates = true;
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::setColumns(const QVariantList &columns) {
    QVariantList normalized;
    normalized.reserve(columns.size());
    for (const QVariant &column : columns) normalized.append(unpack(column));
    if (m_columns == normalized) return;
    m_columns = std::move(normalized);
    m_widthErrors.clear();
    if (!m_cppModel)
        for (auto &record : m_createdDelegateItems) record.allRolesDirty = true;
    setCurrentColumn(m_currentColumn);
    emit columnsChanged();
    forceLayout();
}
void TableView::clearSelection() {
    const Position previous{m_selectedRow, m_currentColumn};
    const bool rowChanged = m_selectedRow != -1;
    const bool columnChanged = m_currentColumn != -1;
    m_selectedRow = -1;
    m_currentColumn = -1;
    m_currentIndex = QPersistentModelIndex();
    if (rowChanged) emit selectedRowChanged();
    if (columnChanged) emit currentColumnChanged();
    refreshDelegateStates(previous);
}
void TableView::setModel(const QVariant &model) {
    if (m_modelVariant == model) return;
    if (m_cppModel) disconnect(m_cppModel, nullptr, this, nullptr);
    m_modelVariant = model;
    m_listModel.clear();
    m_cppModel.clear();
    m_roleNames.clear();
    if (model.metaType() == QMetaType::fromType<QJSValue>()) m_listModel = unpack(model).toList();
    else if (model.canConvert<QVariantList>()) m_listModel = model.toList();
    else m_cppModel = qobject_cast<QAbstractItemModel *>(model.value<QObject *>());
    if (m_selectedRow >= 0) {
        if (rowCount() > 0) {
            const int newRow = std::clamp(m_selectedRow, 0, rowCount() - 1);
            const bool rowChanged = (m_selectedRow != newRow);
            m_selectedRow = newRow;
            m_currentIndex = sourceIndex(m_selectedRow, (std::max)(0, m_currentColumn));
            if (rowChanged) emit selectedRowChanged();
        } else {
            clearSelection();
        }
    }
    if (m_cppModel) {
        m_roleNames = m_cppModel->roleNames();
        connect(m_cppModel, &QObject::destroyed, this, [this] {
            m_cppModel.clear();
            m_modelVariant.clear();
            m_roleNames.clear();
            clearSelection();
            generateDelegateItems();
            forceLayout();
            emit modelChanged();
        });
        connect(m_cppModel, &QAbstractItemModel::dataChanged, this, &TableView::handleDataChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsInserted, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsRemoved, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsMoved, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsInserted, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsRemoved, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsMoved, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::layoutChanged, this, &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::modelReset, this, [this] {
            m_roleNames = m_cppModel->roleNames();
            m_structureDirty = true;
            if (m_selectedRow >= 0) {
                if (rowCount() > 0) {
                    const int newRow = std::clamp(m_selectedRow, 0, rowCount() - 1);
                    const bool rowChanged = (m_selectedRow != newRow);
                    m_selectedRow = newRow;
                    m_currentIndex = sourceIndex(m_selectedRow, (std::max)(0, m_currentColumn));
                    if (rowChanged) emit selectedRowChanged();
                } else {
                    clearSelection();
                }
            }
            generateDelegateItems();
            forceLayout();
        });
    }
    generateDelegateItems();
    forceLayout();
    emit modelChanged();
}
int TableView::rowCount() const { return m_cppModel ? m_cppModel->rowCount() : int(m_listModel.size()); }
int TableView::columnCount() const { return m_columns.isEmpty() ? 5 : int(m_columns.size()); }
int TableView::headerRows() const { return !m_columns.isEmpty() || m_columnWidthProvider.isCallable() ? 2 : 0; }
QModelIndex TableView::sourceIndex(int row, int column) const {
    if (!m_cppModel || row < 0 || row >= rowCount()) return {};
    QModelIndex index = m_cppModel->index(row, column);
    if (!index.isValid() && column > 0) index = m_cppModel->index(row, 0);
    return index;
}
void TableView::setSelectedRow(int row) {
    const int normalized = row < 0 || rowCount() == 0 ? -1 : (std::min)(row, rowCount() - 1);
    if (m_selectedRow == normalized) return;
    const Position previous{m_selectedRow, m_currentColumn};
    m_selectedRow = normalized;
    if (normalized >= 0 && m_currentColumn < 0) {
        m_currentColumn = 0;
        emit currentColumnChanged();
    }
    m_currentIndex = sourceIndex(m_selectedRow, (std::max)(0, m_currentColumn));
    emit selectedRowChanged();
    refreshDelegateStates(previous);
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::setCurrentColumn(int column) {
    const int normalized = column < 0 ? -1 : (std::min)(column, columnCount() - 1);
    if (m_currentColumn == normalized) return;
    const Position previous{m_selectedRow, m_currentColumn};
    m_currentColumn = normalized;
    m_currentIndex = sourceIndex(m_selectedRow, (std::max)(0, m_currentColumn));
    emit currentColumnChanged();
    refreshDelegateStates(previous);
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::synchronizeSelection() {
    if (m_selectedRow < 0) return;
    if (rowCount() == 0) {
        clearSelection();
        return;
    }
    if (m_currentIndex.isValid() && !m_currentIndex.parent().isValid()) {
        const int newRow = m_currentIndex.row();
        if (newRow >= 0 && newRow < rowCount()) {
            if (m_selectedRow != newRow) {
                m_selectedRow = newRow;
                emit selectedRowChanged();
            }
        } else {
            clearSelection();
            return;
        }
    } else if (m_cppModel) {
        clearSelection();
        return;
    } else {
        const int newRow = std::clamp(m_selectedRow, 0, rowCount() - 1);
        if (m_selectedRow != newRow) {
            m_selectedRow = newRow;
            emit selectedRowChanged();
        }
    }
    // List models use visual columns backed by source column zero.
    if (m_cppModel && m_cppModel->columnCount() > 1 && m_currentColumn >= 0) {
        const int column = (std::min)(m_currentIndex.column(), columnCount() - 1);
        if (m_currentColumn != column) {
            m_currentColumn = column;
            emit currentColumnChanged();
        }
    }
}
void TableView::handleStructureChanged() {
    m_structureDirty = true;
    synchronizeSelection();
    forceLayout();
}
void TableView::setAlternatingRows(bool alternating) {
    if (m_alternatingRows == alternating) return;
    m_alternatingRows = alternating;
    emit alternatingRowsChanged();
    markDirty();
}
void TableView::setHeaderColor(Color color) {
    if (m_headerColor == color) return;
    m_headerColor = color;
    m_resolvedHeaderColor = ColorMapper::fromString(m_headerColor, 0xFFFFFF00);
    emit headerColorChanged();
    markDirty();
}

void TableView::setHeaderBackgroundColor(Color color) {
    if (m_headerBackgroundColor == color) return;
    m_headerBackgroundColor = color;
    m_resolvedHeaderBackgroundColor = ColorMapper::fromString(m_headerBackgroundColor, 0x00000000);
    emit headerBackgroundColorChanged();
    markDirty();
}

void TableView::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0xFFFFFFFF);
    emit selectionColorChanged();
    markDirty();
}

void TableView::setSelectionBackgroundColor(Color color) {
    if (m_selectionBackgroundColor == color) return;
    m_selectionBackgroundColor = color;
    m_resolvedSelectionBackgroundColor = ColorMapper::fromString(m_selectionBackgroundColor, 0xFF0000FF);
    emit selectionBackgroundColorChanged();
    markDirty();
}

void TableView::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    m_resolvedTextColor = ColorMapper::fromString(m_textColor, 0xFFCCCCCC);
    emit textColorChanged();
    markDirty();
}

void TableView::setRowBackgroundColor(Color color) {
    if (m_rowBackgroundColor == color) return;
    m_rowBackgroundColor = color;
    m_resolvedRowBackgroundColor = ColorMapper::fromString(m_rowBackgroundColor, 0x00000000);
    emit rowBackgroundColorChanged();
    markDirty();
}

void TableView::setAlternatingRowBackgroundColor(Color color) {
    if (m_alternatingRowBackgroundColor == color) return;
    m_alternatingRowBackgroundColor = color;
    m_resolvedAltRowBackgroundColor = ColorMapper::fromString(m_alternatingRowBackgroundColor, 0xFF1E1E2E);
    emit alternatingRowBackgroundColorChanged();
    markDirty();
}

void TableView::setShowGrid(bool show) {
    if (m_showGrid == show) return;
    m_showGrid = show;
    emit showGridChanged();
    markDirty();
}



void TableView::forceLayout() {
    m_geometryDirty = true;
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::setColumnWidthProvider(const QJSValue &provider) {
    m_columnWidthProvider = provider;
    m_widthErrors.clear();
    emit columnWidthProviderChanged();
    forceLayout();
}
void TableView::setCellFormat(const QJSValue &format) {
    m_cellFormat = format;
    m_formatErrors.clear();
    emit cellFormatChanged();
    markDirty();
}
void TableView::setDelegate(QQmlComponent *delegate) {
    if (m_delegate == delegate) return;
    generateDelegateItems();
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            m_delegate.clear();
            generateDelegateItems();
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
    generateDelegateItems();
    if (m_rowDelegate) disconnect(m_rowDelegate, nullptr, this, nullptr);
    m_rowDelegate = delegate;
    if (m_rowDelegate) {
        connect(m_rowDelegate, &QObject::destroyed, this, [this]() {
            m_rowDelegate.clear();
            generateDelegateItems();
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


void TableView::callbackWarning(const QString &message, QSet<QString> &errors) const {
    if (!errors.contains(message) && errors.size() < 16) {
        errors.insert(message);
        qmlWarning(this) << message;
    }
}
int TableView::resolveColumnWidth(int column, const QVariantMap &descriptor) {
    const auto valid = [](double value) {
        return std::isfinite(value) && value >= 0 && std::floor(value) == value
            && value <= (std::numeric_limits<int>::max)();
    };
    if (m_columnWidthProvider.isCallable()) {
        const QJSValue result = m_columnWidthProvider.call({column});
        if (result.isError()) {
            callbackWarning(QStringLiteral("columnWidthProvider: ") + result.toString()
                            + QLatin1Char('\n') + result.property("stack").toString(), m_widthErrors);
        } else if (result.isNumber() && valid(result.toNumber())) {
            m_widthErrors.clear();
            return int(result.toNumber());
        } else if (!result.isUndefined() && !(result.isNumber() && std::isfinite(result.toNumber()) && result.toNumber() < 0)) {
            callbackWarning(QStringLiteral("columnWidthProvider must return a finite integer width"), m_widthErrors);
        } else {
            m_widthErrors.clear();
        }
    }
    bool converted = false;
    const double width = descriptor.value(QStringLiteral("width"), 12).toDouble(&converted);
    if (converted && valid(width)) return int(width);
    callbackWarning(QStringLiteral("Invalid TableView column width; using 12"), m_widthErrors);
    return 12;
}
void TableView::resolveColumns() {
    if (!m_geometryDirty) return;
    m_geometryDirty = false;
    m_geometry.clear();
    if (width() <= 0 || height() <= 0) return;
    qint64 x = 0;
    bool overscan = false;
    for (int column = 0; column < columnCount(); ++column) {
        const QVariant value = column < m_columns.size() ? unpack(m_columns[column]) : QVariant();
        const QVariantMap descriptor = value.toMap();
        const int columnWidth = resolveColumnWidth(column, descriptor);
        if (columnWidth == 0) continue;
        const QString defaultTitle = QStringLiteral("Col%1").arg(column + 1);
        const QString title = value.canConvert<QVariantMap>()
            ? descriptor.value(QStringLiteral("title"), defaultTitle).toString()
            : value.isValid() ? value.toString() : defaultTitle;
        m_geometry.append({column, int(x), columnWidth, title,
            descriptor.value(QStringLiteral("alignment"),
                descriptor.value(QStringLiteral("align"), QStringLiteral("left"))).toString()});
        if (overscan) break;
        x += columnWidth;
        if (x >= width()) {
            if (x > (std::numeric_limits<int>::max)()) break;
            overscan = true;
        }
    }
}
QString TableView::cellText(int row, int col) const {
    if (m_cppModel) {
        const QModelIndex idx = sourceIndex(row, col);
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




void TableView::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                  const QList<int> &roles) {
    m_fetchedRoles.clear();
    bool affected = false;
    for (auto &record : m_createdDelegateItems) {
        const QModelIndex index = record.source;
        if (!topLeft.isValid() || !bottomRight.isValid()
            || (index.isValid() && index.parent() == topLeft.parent()
                && index.row() >= topLeft.row() && index.row() <= bottomRight.row()
                && index.column() >= topLeft.column() && index.column() <= bottomRight.column())) {
            if (roles.isEmpty()) record.allRolesDirty = true;
            else for (int role : roles) record.dirtyRoles.insert(role);
            affected = true;
        }
    }
    if (affected) markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    else if (!m_delegate && !m_rowDelegate) markDirty();
}
void TableView::updateDelegateState(DelegateRecord &record, int row, int column) {
    const bool current = row == m_selectedRow && column == m_currentColumn;
    if (!record.item || record.current == current) return;
    record.current = current;
    QQmlContext *context = delegateContext(record.item);
    for (const char *name : {"current", "selected"}) {
        DelegateUtils::setContextProperty(record.component, context, QString::fromLatin1(name), current);
        DelegateUtils::setPropertyIfExists(record.item, name, current);
    }
}
void TableView::refreshDelegateStates(Position previous) {
    for (const Position position : {previous, Position{m_selectedRow, m_currentColumn}}) {
        auto it = m_createdDelegateItems.find(position);
        if (it != m_createdDelegateItems.end())
            updateDelegateState(it.value(), position.first, position.second);
    }
}
void TableView::updateDelegate(DelegateRecord &record, int row, int column) {
    const bool cellMode = bool(m_delegate);
    const bool dataDirty = !record.loaded || record.allRolesDirty || !record.dirtyRoles.isEmpty();
    const bool allRolesDirty = record.allRolesDirty;
    const QSet<int> dirtyRoles = std::exchange(record.dirtyRoles, {});
    record.allRolesDirty = false;
    const bool moved = record.row != row || record.column != column;
    QVariantMap values = record.values;
    QVariant modelData = record.modelData;
    if (dataDirty) {
        if (m_cppModel) {
            auto &fetched = m_fetchedRoles[record.source];
            const auto readRole = [&](int role) {
                if (!fetched.contains(role)) fetched.insert(role, m_cppModel->data(record.source, role));
                return fetched.value(role);
            };
            if (allRolesDirty || !record.loaded) values.clear();
            for (auto it = m_roleNames.cbegin(); it != m_roleNames.cend(); ++it)
                if (allRolesDirty || !record.loaded || dirtyRoles.contains(it.key()))
                    values.insert(QString::fromUtf8(it.value()), readRole(it.key()));
            modelData = m_roleNames.isEmpty() ? readRole(Qt::DisplayRole) : QVariant(values);
        } else {
            const QVariant value = row >= 0 && row < m_listModel.size() ? unpack(m_listModel[row]) : QVariant();
            values = value.toMap();
            modelData = cellMode ? QVariant(cellText(row, column)) : value;
            if (cellMode) values.insert(QStringLiteral("display"), modelData);
        }
    }
    QQmlComponent *raw = m_delegate ? m_delegate.data() : m_rowDelegate.data();
    QQmlComponent *component = record.component;
    if (dataDirty || moved || !record.item)
        component = DelegateUtils::resolveDelegate(raw, row, column, values);
    if (record.component != component) destroyDelegate(record);
    const auto assign = [&](const QString &name, const QVariant &value) {
        DelegateUtils::setContextProperty(component, delegateContext(record.item), name, value);
        DelegateUtils::setPropertyIfExists(record.item, name.toUtf8().constData(), value);
    };
    const qint64 index = cellMode ? qint64(row) * columnCount() + column : row;
    if (!record.item && DelegateUtils::isReady(component, this)) {
        QVariantMap initial = values;
        initial.insert(QStringLiteral("index"), index);
        initial.insert(QStringLiteral("row"), row);
        if (cellMode) initial.insert(QStringLiteral("column"), column);
        record.current = row == m_selectedRow && column == m_currentColumn;
        initial.insert(QStringLiteral("current"), record.current);
        initial.insert(QStringLiteral("selected"), record.current);
        initial.insert(QStringLiteral("model"), values);
        initial.insert(QStringLiteral("modelData"), modelData);
        if (QQmlContext *context = DelegateUtils::createContext(component, this)) {
            for (auto it = initial.cbegin(); it != initial.cend(); ++it)
                DelegateUtils::setContextProperty(component, context, it.key(), it.value());
            record.item = DelegateUtils::createItem(component, initial, context, this, this);
            record.component = component;
            if (record.item && record.item->height() <= 0) record.item->setHeight(1);
        }
    } else if (record.item) {
        for (auto it = values.cbegin(); it != values.cend(); ++it)
            if (record.values.value(it.key()) != it.value()) assign(it.key(), it.value());
        for (auto it = record.values.cbegin(); it != record.values.cend(); ++it)
            if (!values.contains(it.key())) assign(it.key(), QVariant());
        if (record.values != values) assign(QStringLiteral("model"), values);
        if (record.modelData != modelData) assign(QStringLiteral("modelData"), modelData);
        if (moved) {
            assign(QStringLiteral("row"), row);
            if (cellMode) assign(QStringLiteral("column"), column);
        }
        if (record.item->property("index").toLongLong() != index) assign(QStringLiteral("index"), index);
        updateDelegateState(record, row, column);
    }
    record.row = row;
    record.column = column;
    record.values = values;
    record.modelData = modelData;
    record.loaded = true;
}
int TableView::computeFirstVisibleRow() const {
    const int visibleRows = (std::max)(1, height() - headerRows());
    const int first = m_selectedRow >= visibleRows ? m_selectedRow - visibleRows + 1 : 0;
    return std::clamp(first, 0, (std::max)(0, rowCount() - visibleRows));
}
void TableView::updateLayout() {
    Item::updateLayout();
    resolveColumns();
    m_firstVisibleRow = computeFirstVisibleRow();
    layoutDelegates();
}
void TableView::layoutDelegates() {
    if (m_resetDelegates || (!m_delegate && !m_rowDelegate)) {
        clearDelegateItems();
        m_resetDelegates = false;
    }
    if (!m_delegate && !m_rowDelegate) return;
    if (m_structureDirty) {
        QHash<Position, DelegateRecord> remapped;
        for (auto it = m_createdDelegateItems.begin(); it != m_createdDelegateItems.end(); ++it) {
            auto &record = it.value();
            if (!record.source.isValid() || record.source.parent().isValid()) {
                destroyDelegate(record);
                continue;
            }
            const int column = m_delegate && m_cppModel->columnCount() > 1
                ? record.source.column() : it.key().second;
            const Position position{record.source.row(), column};
            if (remapped.contains(position)) {
                destroyDelegate(record);
                continue;
            }
            remapped.insert(position, std::move(record));
        }
        m_createdDelegateItems = std::move(remapped);
        m_structureDirty = false;
    }
    QHash<Position, ColumnGeometry> wanted;
    const int bodyHeight = (std::max)(0, height() - headerRows());
    if (width() > 0 && bodyHeight > 0) {
        const int first = (std::max)(0, m_firstVisibleRow - 2);
        const int last = int((std::min)(qint64(rowCount()) - 1, qint64(m_firstVisibleRow) + bodyHeight + 1));
        for (int row = first; row <= last; ++row) {
            if (m_delegate) {
                for (const auto &geometry : std::as_const(m_geometry))
                    wanted.insert({row, geometry.column}, geometry);
            } else wanted.insert({row, 0}, {0, 0, width(), {}, {}});
        }
    }
    for (auto it = m_createdDelegateItems.begin(); it != m_createdDelegateItems.end();) {
        if (!wanted.contains(it.key())) {
            destroyDelegate(it.value());
            it = m_createdDelegateItems.erase(it);
        } else ++it;
    }
    m_fetchedRoles.clear();
    for (auto it = wanted.cbegin(); it != wanted.cend(); ++it) {
        auto &record = m_createdDelegateItems[it.key()];
        const int row = it.key().first;
        const int column = it.key().second;
        const QModelIndex source = sourceIndex(row, m_delegate ? column : 0);
        if (record.source != source) {
            record.source = source;
            record.allRolesDirty = true;
        }
        updateDelegate(record, row, column);
        if (record.item) {
            const auto &geometry = it.value();
            const bool visible = row >= m_firstVisibleRow && qint64(row) < qint64(m_firstVisibleRow) + bodyHeight
                && geometry.x < width();
            // Hidden overscan items need no potentially huge absolute origin.
            record.item->setX(visible ? geometry.x : 0);
            record.item->setY(headerRows() + row - m_firstVisibleRow);
            record.item->setWidth(geometry.width);
            record.item->setVisible(visible);
        }
    }
    m_fetchedRoles.clear();
}

void TableView::paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const {
    Q_UNUSED(clipRect);
    if (!m_visible || width() <= 0 || height() <= 0) return;
    if (headerRows()) {
        for (const auto &geometry : std::as_const(m_geometry)) {
            if (geometry.x >= width()) break;
            const int visibleWidth = (std::min)(geometry.width, width() - geometry.x);
            buffer.writeText(absX + geometry.x, absY,
                alignedPrefix(geometry.title, geometry.width, visibleWidth, geometry.alignment),
                m_resolvedHeaderColor, m_resolvedHeaderBackgroundColor, TextAttribute::Bold);
            if (m_showGrid && geometry.column < columnCount() - 1 && geometry.width < width() - geometry.x)
                if (Cell *cell = buffer.tryCellAt(absX + geometry.x + geometry.width - 1, absY))
                    cell->setCharacter("|");
        }
        if (height() > 1)
            buffer.drawLine(absX, absY + 1, absX + width() - 1, absY + 1, "─", m_resolvedHeaderColor);
    }
    if (m_delegate || m_rowDelegate) return;
    const int bodyHeight = (std::max)(0, height() - headerRows());
    for (int r = 0; r < bodyHeight && qint64(m_firstVisibleRow) + r < rowCount(); ++r) {
        const int row = m_firstVisibleRow + r;
        const int y = absY + headerRows() + r;
        const bool selected = row == m_selectedRow;
        const uint32_t bg = selected ? m_resolvedSelectionBackgroundColor :
            m_alternatingRows && row % 2 ? m_resolvedAltRowBackgroundColor : m_resolvedRowBackgroundColor;
        const uint32_t fg = selected ? m_resolvedSelectionColor : m_resolvedTextColor;
        const TextAttribute attributes = selected ? TextAttribute::Bold : TextAttribute::None;
        if (selected || bg != 0) buffer.fillRect(absX, y, width(), 1, " ", fg, bg);
        for (const auto &geometry : std::as_const(m_geometry)) {
            if (geometry.x >= width()) break;
            QString value = cellText(row, geometry.column);
            QString alignment = geometry.alignment;
            uint32_t cellFg = fg;
            uint32_t cellBg = bg;
            TextAttribute cellAttributes = attributes;
            if (m_cellFormat.isCallable()) {
                const QJSValue result = m_cellFormat.call({row, geometry.column, value});
                if (result.isError()) {
                    callbackWarning(QStringLiteral("cellFormat: ") + result.toString()
                                    + QLatin1Char('\n') + result.property("stack").toString(), m_formatErrors);
                } else {
                    m_formatErrors.clear();
                    if (result.isObject()) {
                        if (result.hasProperty("text")) value = result.property("text").toString();
                        if (result.hasProperty("color")) cellFg = ColorMapper::fromString(result.property("color").toString(), cellFg);
                        if (result.hasProperty("backgroundColor")) cellBg = ColorMapper::fromString(result.property("backgroundColor").toString(), cellBg);
                        if (result.property("bold").toBool()) cellAttributes = TextAttribute::Bold;
                        if (result.hasProperty("align")) alignment = result.property("align").toString();
                    }
                }
            }
            const int visibleWidth = (std::min)(geometry.width, width() - geometry.x);
            buffer.writeText(absX + geometry.x, y,
                alignedPrefix(value, geometry.width, visibleWidth, alignment), cellFg, cellBg, cellAttributes);
            if (m_showGrid && geometry.column < columnCount() - 1 && geometry.width < width() - geometry.x)
                if (Cell *cell = buffer.tryCellAt(absX + geometry.x + geometry.width - 1, y))
                    cell->setCharacter("|");
        }
    }
}
ClipRect TableView::clipRectForChildren(int absX, int absY, const ClipRect &clipRect) const {
    return clipRect.intersected({absX, absY + headerRows(), (std::max)(0, width()),
                                (std::max)(0, height() - headerRows())});
}
void TableView::keyPressEvent(KeyEvent *event) {
    const int page = (std::max)(1, height() - headerRows());
    if (event->key() == Qt::Key_Up) setSelectedRow(m_selectedRow - 1);
    else if (event->key() == Qt::Key_Down) setSelectedRow(m_selectedRow + 1);
    else if (event->key() == Qt::Key_PageUp) setSelectedRow(m_selectedRow - page);
    else if (event->key() == Qt::Key_PageDown)
        setSelectedRow(int((std::min)(qint64(m_selectedRow) + page, qint64(rowCount()) - 1)));
    else if (event->key() == Qt::Key_Right) setCurrentColumn(m_currentColumn + 1);
    else if (event->key() == Qt::Key_Left) setCurrentColumn(m_currentColumn - 1);
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
        if (m_selectedRow >= 0 && m_selectedRow < rowCount()) emit rowActivated(m_selectedRow);
    } else return;
    event->setAccepted(true);
}
void TableView::mousePressEvent(MouseEvent *event) {
    if (event->button() != 0 || event->x() < 0 || event->x() >= width()
        || event->y() < 0 || event->y() >= height()) return;
    setFocus(true);
    int column = -1;
    for (const auto &geometry : std::as_const(m_geometry))
        if (event->x() >= geometry.x && qint64(event->x()) < qint64(geometry.x) + geometry.width) {
            column = geometry.column;
            break;
        }
    if (column < 0) return;
    if (headerRows() && event->y() == 0) {
        emit headerClicked(column);
        event->setAccepted(true);
        return;
    }
    if (event->y() < headerRows()) return;
    const int row = computeFirstVisibleRow() + event->y() - headerRows();
    if (row >= 0 && row < rowCount()) {
        setCurrentColumn(column);
        setSelectedRow(row);
        emit rowActivated(row);
        event->setAccepted(true);
    }
}
} // namespace qtui
