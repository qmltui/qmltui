#include <QmlTui/Core/Items/TableView.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/NumericItemRoles_p.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlInfo>
#include <QQmlListReference>
#include <QVarLengthArray>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qmltui {
namespace {
enum GridGlyph : uint8_t {
    Horizontal,
    Vertical,
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    TopTee,
    BottomTee,
    LeftTee,
    RightTee,
    Cross,
};

struct RegisterTableConverters {
    RegisterTableConverters() {
        QMetaType::registerConverter<QString, TableView::GridStyle>([](const QString& str) {
            return TableView::gridStyleFromString(str, TableView::Single);
        });
        QMetaType::registerConverter<TableView::GridStyle, QString>(
            [](TableView::GridStyle style) { return TableView::gridStyleToString(style); });
    }
} s_registerTableConverters;

QQmlContext* delegateContext(Item* item) {
    return item ? item->findChild<QQmlContext*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}
QVariant unpack(QVariant value) {
    return value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant()
                                                               : value;
}
// Compute alignment against the logical width, but allocate only the visible prefix.
QString alignedPrefix(const QString& text, int width, int visibleWidth, const QString& alignment,
                      int leftPadding = 0, int rightPadding = 0) {
    if (width <= 0 || visibleWidth <= 0) return {};
    int leftPad = (std::max)(0, leftPadding);
    int rightPad = (std::max)(0, rightPadding);
    if (leftPad > width) leftPad = width;
    if (rightPad > width - leftPad) rightPad = width - leftPad;
    const int innerWidth = width - leftPad - rightPad;

    int used = 0;
    GraphemeIterator measure(text);
    GraphemeRef cluster;
    while (measure.next(cluster)) {
        if (cluster.width > innerWidth - used) break;
        used += cluster.width;
    }
    const int extraSpace = innerWidth - used;
    const int alignOffset = alignment == "right"    ? extraSpace
                            : alignment == "center" ? extraSpace / 2
                                                    : 0;
    const int left = leftPad + alignOffset;

    QString result((std::min)(left, visibleWidth), QLatin1Char(' '));
    int position = left;
    GraphemeIterator draw(text);
    while (position < visibleWidth && draw.next(cluster)) {
        if (cluster.width > width - rightPad - position || cluster.width > visibleWidth - position)
            break;
        result.append(cluster.utf16);
        position += cluster.width;
    }
    if (position < visibleWidth) result.append(QString(visibleWidth - position, QLatin1Char(' ')));
    return result;
}
} // namespace

TableView::TableView(QObject* parent) : Item(parent) {
    setFocusPolicy(Qt::StrongFocus);
    connect(this, &Item::widthChanged, this, &TableView::forceLayout);
    connect(this, &Item::heightChanged, this, &TableView::forceLayout);
    forceLayout();
}
int TableView::implicitWidth() const {
    if (m_implicitWidth > 0) return m_implicitWidth;
    return m_calculatedImplicitWidth;
}
TableView::~TableView() {
    clearDelegateItems();
}
void TableView::destroyDelegate(DelegateRecord& record) {
    if (record.item) {
        record.item->setParentItem(nullptr);
        delete record.item.data();
    }
    delete record.container.data();
    record.container.clear();
    record.item.clear();
    record.component.clear();
}
void TableView::clearDelegateItems() {
    for (auto& record : m_createdDelegateItems)
        destroyDelegate(record);
    m_createdDelegateItems.clear();
}
void TableView::generateDelegateItems() {
    m_resetDelegates = true;
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::setColumns(const QVariantList& columns) {
    QVariantList normalized;
    normalized.reserve(columns.size());
    for (const QVariant& column : columns)
        normalized.append(unpack(column));
    if (m_columns == normalized) return;
    m_columns = std::move(normalized);
    m_widthErrors.clear();
    m_builtInCells.clear();
    updateImplicitWidth();
    refreshModelHeader();
    if (!m_cppModel)
        for (auto& record : m_createdDelegateItems)
            record.allRolesDirty = true;
    setCurrentColumn(m_currentColumn);
    emit columnsChanged();
    forceLayout();
}
void TableView::resetViewport() {
    m_firstVisibleRow = 0;
    m_topIndex = QPersistentModelIndex();
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
void TableView::setModel(const QVariant& model) {
    if (m_modelVariant == model) return;
    if (m_cppModel) disconnect(m_cppModel, nullptr, this, nullptr);
    m_modelVariant = model;
    m_listModel.clear();
    m_cppModel.clear();
    m_roleNames.clear();
    m_roleNameStrings.clear();
    m_claimedNames = 0;
    m_builtInCells.clear();
    if (model.metaType() == QMetaType::fromType<QJSValue>())
        m_listModel = unpack(model).toList();
    else if (model.canConvert<QVariantList>())
        m_listModel = model.toList();
    else
        m_cppModel = qobject_cast<QAbstractItemModel*>(model.value<QObject*>());
    resetViewport();
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
        refreshModelRoles();
        connect(m_cppModel, &QObject::destroyed, this, [this] {
            m_cppModel.clear();
            m_modelVariant.clear();
            m_roleNames.clear();
            m_roleNameStrings.clear();
            m_claimedNames = 0;
            m_builtInCells.clear();
            m_modelProvidesHeader = false;
            resetViewport();
            clearSelection();
            updateImplicitWidth();
            generateDelegateItems();
            forceLayout();
            emit modelChanged();
        });
        connect(m_cppModel, &QAbstractItemModel::dataChanged, this, &TableView::handleDataChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsInserted, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsRemoved, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::rowsMoved, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsInserted, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsRemoved, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::columnsMoved, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::layoutChanged, this,
                &TableView::handleStructureChanged);
        connect(m_cppModel, &QAbstractItemModel::headerDataChanged, this,
                [this](Qt::Orientation orientation) {
                    if (orientation != Qt::Horizontal) return;
                    refreshModelHeader();
                    forceLayout();
                });
        connect(m_cppModel, &QAbstractItemModel::modelReset, this, [this] {
            refreshModelRoles();
            refreshModelHeader();
            m_structureDirty = true;
            m_builtInCells.clear();
            resetViewport();
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
            updateImplicitWidth();
            generateDelegateItems();
            forceLayout();
        });
    }
    updateImplicitWidth();
    refreshModelHeader();
    generateDelegateItems();
    forceLayout();
    emit modelChanged();
}
int TableView::rowCount() const {
    return m_cppModel ? m_cppModel->rowCount() : int(m_listModel.size());
}
int TableView::columnCount() const {
    if (!m_columns.isEmpty()) return int(m_columns.size());
    // Without column descriptors a C++ model defines its own column count;
    // JS lists keep the historical default of five columns.
    return m_cppModel ? (std::max)(1, m_cppModel->columnCount()) : 5;
}
int TableView::headerRows() const {
    return !m_columns.isEmpty() || m_columnWidthProvider.isCallable() || m_modelProvidesHeader ? 2
                                                                                               : 0;
}
QVariant TableView::modelHeader(int column, int role) const {
    return m_cppModel ? m_cppModel->headerData(column, Qt::Horizontal, role) : QVariant();
}
void TableView::refreshModelRoles() {
    m_roleNames = m_cppModel->roleNames();
    m_roleNameStrings.clear();
    m_roleNameStrings.reserve(m_roleNames.size());
    for (auto it = m_roleNames.cbegin(); it != m_roleNames.cend(); ++it)
        m_roleNameStrings.insert(it.key(), QString::fromUtf8(it.value()));
    m_claimedNames = NumericItemRoles::claimedNames(m_cppModel.data());
    // Roles beyond the ones QAbstractItemModel supplies by default, which is
    // what a model actually meant to publish to QML.
}
void TableView::refreshModelHeader() {
    // QAbstractItemModel::headerData() falls back to the section number, so a
    // model "provides" a header only when at least one section is a string.
    bool provides = false;
    if (m_cppModel && m_columns.isEmpty()) {
        const int count = columnCount();
        for (int column = 0; column < count && !provides; ++column) {
            const QVariant title = modelHeader(column, Qt::DisplayRole);
            provides =
                title.metaType() == QMetaType::fromType<QString>() && !title.toString().isEmpty();
        }
    }
    m_modelProvidesHeader = provides;
}
QString TableView::alignmentName(const QVariant& alignment) {
    return NumericItemRoles::alignmentName(alignment);
}
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
    m_builtInCells.clear();
    updateImplicitWidth();
    refreshModelHeader();
    // Before the selection: synchronizeSelection() returns early when nothing
    // is selected, and the viewport still has to be fixed up in that case.
    synchronizeViewport();
    synchronizeSelection();
    forceLayout();
}
void TableView::setSelectionKeepsCellColors(bool keep) {
    if (m_selectionKeepsCellColors == keep) return;
    m_selectionKeepsCellColors = keep;
    emit selectionKeepsCellColorsChanged();
    markDirty(DirtyFlag::SelfDirty);
}
void TableView::setAlternatingRows(bool alternating) {
    if (m_alternatingRows == alternating) return;
    m_alternatingRows = alternating;
    emit alternatingRowsChanged();
    markDirty();
}
void TableView::setStretchLastColumn(bool stretch) {
    if (m_stretchLastColumn == stretch) return;
    m_stretchLastColumn = stretch;
    emit stretchLastColumnChanged();
    forceLayout();
}
void TableView::setHeaderColor(Color color) {
    if (m_headerColor == color) return;
    m_headerColor = color;
    m_resolvedHeaderColor = Color::resolve(m_headerColor, 0);
    emit headerColorChanged();
    markDirty();
}

void TableView::setHeaderBackgroundColor(Color color) {
    if (m_headerBackgroundColor == color) return;
    m_headerBackgroundColor = color;
    m_resolvedHeaderBackgroundColor = Color::resolve(m_headerBackgroundColor, 0);
    emit headerBackgroundColorChanged();
    markDirty();
}

void TableView::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    m_resolvedSelectionColor = Color::resolve(m_selectionColor, 0);
    emit selectionColorChanged();
    markDirty();
}

void TableView::setSelectionBackgroundColor(Color color) {
    if (m_selectionBackgroundColor == color) return;
    m_selectionBackgroundColor = color;
    m_resolvedSelectionBackgroundColor = Color::resolve(m_selectionBackgroundColor, 0);
    emit selectionBackgroundColorChanged();
    markDirty();
}

void TableView::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    m_resolvedTextColor = Color::resolve(m_textColor, 0);
    emit textColorChanged();
    markDirty();
}

void TableView::setRowBackgroundColor(Color color) {
    if (m_rowBackgroundColor == color) return;
    m_rowBackgroundColor = color;
    m_resolvedRowBackgroundColor = Color::resolve(m_rowBackgroundColor, 0);
    emit rowBackgroundColorChanged();
    markDirty();
}

void TableView::setAlternatingRowBackgroundColor(Color color) {
    if (m_alternatingRowBackgroundColor == color) return;
    m_alternatingRowBackgroundColor = color;
    m_resolvedAltRowBackgroundColor = Color::resolve(m_alternatingRowBackgroundColor, 0);
    emit alternatingRowBackgroundColorChanged();
    markDirty();
}

void TableView::setGridColor(Color color) {
    if (m_gridColor == color) return;
    m_gridColor = color;
    m_resolvedGridColor = Color::resolve(m_gridColor, 0);
    emit gridColorChanged();
    markDirty();
}

void TableView::setShowColumnGrid(bool show) {
    if (m_showColumnGrid == show) return;
    m_showColumnGrid = show;
    emit showColumnGridChanged();
    updateImplicitWidth();
    forceLayout();
}

void TableView::setShowRowGrid(bool show) {
    if (m_showRowGrid == show) return;
    m_showRowGrid = show;
    emit showRowGridChanged();
    forceLayout();
}

void TableView::setShowOuterBorder(bool show) {
    if (m_showOuterBorder == show) return;
    m_showOuterBorder = show;
    emit showOuterBorderChanged();
    updateImplicitWidth();
    forceLayout();
}

void TableView::setGridStyle(GridStyle style) {
    if (m_gridStyle == style) return;
    m_gridStyle = style;
    emit gridStyleChanged();
    markDirty();
}

void TableView::setGridStyle(const QString& style) {
    if (style == QLatin1String("single")) {
        setGridStyle(Single);
    } else if (style == QLatin1String("double")) {
        setGridStyle(Double);
    } else if (style == QLatin1String("ascii")) {
        setGridStyle(Ascii);
    } else {
        qmlWarning(this) << "Unsupported TableView gridStyle:" << style;
    }
}

TableView::GridStyle TableView::gridStyleFromString(QStringView str, GridStyle defaultStyle) {
    if (str.compare(u"single", Qt::CaseInsensitive) == 0) return Single;
    if (str.compare(u"double", Qt::CaseInsensitive) == 0) return Double;
    if (str.compare(u"ascii", Qt::CaseInsensitive) == 0) return Ascii;
    return defaultStyle;
}

QString TableView::gridStyleToString(GridStyle style) {
    switch (style) {
    case Single:
        return QStringLiteral("single");
    case Double:
        return QStringLiteral("double");
    case Ascii:
        return QStringLiteral("ascii");
    }
    return QStringLiteral("single");
}

void TableView::forceLayout() {
    m_geometryDirty = true;
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
}
void TableView::setColumnWidthProvider(const QJSValue& provider) {
    m_columnWidthProvider = provider;
    m_widthErrors.clear();
    emit columnWidthProviderChanged();
    updateImplicitWidth();
    forceLayout();
}
void TableView::setCellFormat(const QJSValue& format) {
    m_cellFormat = format;
    m_formatErrors.clear();
    m_builtInCells.clear();
    emit cellFormatChanged();
    markDirty();
}
void TableView::setDelegate(QQmlComponent* delegate) {
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

void TableView::setRowDelegate(QQmlComponent* delegate) {
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

void TableView::callbackWarning(const QString& message, QSet<QString>& errors) const {
    if (!errors.contains(message) && errors.size() < 16) {
        errors.insert(message);
        qmlWarning(this) << message;
    }
}
uint32_t TableView::resolveCachedColor(const QString& name, uint32_t fallback) const {
    if (name.isEmpty()) return fallback;
    const auto cached = m_columnColorCache.constFind(name);
    if (cached != m_columnColorCache.cend()) return cached.value();
    const Color parsed = Color::fromString(name);
    if (!parsed.isValid()) return fallback;
    const uint32_t resolved = parsed.argb32();
    // A function that returns a new colour every cell must not grow this
    // without bound; the cache is an optimisation, not a record.
    if (m_columnColorCache.size() >= 256) m_columnColorCache.clear();
    m_columnColorCache.insert(name, resolved);
    return resolved;
}
std::optional<uint32_t> TableView::columnColor(const QJSValue& callback, int row, int column,
                                               const QString& text) const {
    if (!callback.isCallable()) return std::nullopt;

    // Scalars, deliberately: the same (row, column, value) the cellFormat hook
    // takes. Handing the function an object of role values instead cost more
    // than the call itself, and a function that wants model data can ask the
    // model, which QML already has.
    const QJSValue result = callback.call({QJSValue(row), QJSValue(column), QJSValue(text)});
    if (result.isError()) {
        callbackWarning(QStringLiteral("column colour: ") + result.toString() + QLatin1Char('\n') +
                            result.property("stack").toString(),
                        m_columnColorErrors);
        return std::nullopt;
    }
    if (result.isUndefined() || result.isNull()) return std::nullopt;
    const QString name = result.toString();
    if (name.isEmpty()) return std::nullopt;
    return resolveCachedColor(name, 0u);
}
int TableView::resolveColumnWidth(int column, const QVariantMap& descriptor) {
    const auto valid = [](double value) {
        return std::isfinite(value) && value >= 0 && std::floor(value) == value &&
               value <= (std::numeric_limits<int>::max)();
    };
    if (m_columnWidthProvider.isCallable()) {
        const QJSValue result = m_columnWidthProvider.call({column});
        if (result.isError()) {
            callbackWarning(QStringLiteral("columnWidthProvider: ") + result.toString() +
                                QLatin1Char('\n') + result.property("stack").toString(),
                            m_widthErrors);
        } else if (result.isNumber() && valid(result.toNumber())) {
            m_widthErrors.clear();
            return int(result.toNumber());
        } else if (!result.isUndefined() &&
                   !(result.isNumber() && std::isfinite(result.toNumber()) &&
                     result.toNumber() < 0)) {
            callbackWarning(
                QStringLiteral("columnWidthProvider must return a finite integer width"),
                m_widthErrors);
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
void TableView::updateImplicitWidth() {
    int total = 0;
    int visibleColumns = 0;

    const auto validWidth = [](double value) {
        return std::isfinite(value) && value >= 0 && std::floor(value) == value &&
               value <= (std::numeric_limits<int>::max)();
    };

    if (!m_columns.isEmpty()) {
        const int count = int(m_columns.size());
        for (int column = 0; column < count; ++column) {
            const QVariant value = unpack(m_columns[column]);
            const QVariantMap descriptor = value.toMap();
            bool converted = false;
            const double width = descriptor.value(QStringLiteral("width"), 12).toDouble(&converted);
            const int colWidth = (converted && validWidth(width)) ? int(width) : 12;
            if (colWidth <= 0) continue;
            total += colWidth;
            ++visibleColumns;
        }
    } else if (m_cppModel) {
        const int count = (std::max)(0, m_cppModel->columnCount());
        visibleColumns = count;
        total = count * 12;
    } else if (!m_listModel.isEmpty()) {
        const int count = columnCount();
        visibleColumns = count;
        total = count * 12;
    }

    if (visibleColumns > 0) {
        if (m_showColumnGrid && visibleColumns > 1) {
            total += (visibleColumns - 1);
        }
        if (m_showOuterBorder) {
            total += 2;
        }
    }

    if (total != m_calculatedImplicitWidth) {
        m_calculatedImplicitWidth = total;
        emit implicitWidthChanged();
        if (m_width < 0) {
            emit widthChanged();
            markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
        }
    }
}
void TableView::resolveColumns() {
    if (!m_geometryDirty) return;
    m_geometryDirty = false;
    m_geometry.clear();
    m_separators.clear();
    const auto view = viewGeometry();
    if (view.width <= 0 || view.height <= 0) return;
    qint64 x = view.left;
    bool overscan = false;
    const int count = columnCount();
    // A C++ model without descriptors or a width provider shares the width
    // evenly; the last column absorbs the remainder.
    const bool fillFromModel =
        m_columns.isEmpty() && m_cppModel && !m_columnWidthProvider.isCallable();
    const int contentWidth = (std::max)(0, view.width - (m_showColumnGrid ? count - 1 : 0));
    for (int column = 0; column < count; ++column) {
        const QVariant value = column < m_columns.size() ? unpack(m_columns[column]) : QVariant();
        QVariantMap descriptor = value.toMap();
        if (fillFromModel) {
            const int share = (std::max)(1, contentWidth / count);
            descriptor.insert(
                QStringLiteral("width"),
                column == count - 1 ? (std::max)(1, contentWidth - share * (count - 1)) : share);
        }
        int columnWidth = resolveColumnWidth(column, descriptor);
        if (columnWidth == 0) continue;
        if (m_showColumnGrid && !m_geometry.isEmpty()) {
            if (x + 1 < view.right()) m_separators.append(int(x));
            ++x;
        }
        if (x > (std::numeric_limits<int>::max)()) break;

        const bool shouldStretch = (m_stretchLastColumn && column == count - 1) ||
                                   descriptor.value(QStringLiteral("stretch")).toBool();
        if (shouldStretch) {
            int subsequentWidth = 0;
            for (int next = column + 1; next < count; ++next) {
                const QVariant nextVal =
                    next < m_columns.size() ? unpack(m_columns[next]) : QVariant();
                const QVariantMap nextDesc = nextVal.toMap();
                const int nextW = resolveColumnWidth(next, nextDesc);
                if (nextW > 0) {
                    subsequentWidth += nextW;
                    if (m_showColumnGrid) subsequentWidth += 1;
                }
            }
            const int available = int(view.right() - x - subsequentWidth);
            if (descriptor.contains(QStringLiteral("width"))) {
                if (available > columnWidth) columnWidth = available;
            } else {
                columnWidth = (std::max)(1, available);
            }
        }
        QString defaultTitle = QStringLiteral("Col%1").arg(column + 1);
        if (m_modelProvidesHeader) {
            const QVariant header = modelHeader(column, Qt::DisplayRole);
            if (header.metaType() == QMetaType::fromType<QString>() && !header.toString().isEmpty())
                defaultTitle = header.toString();
        }
        const QString title =
            value.canConvert<QVariantMap>()
                ? descriptor.value(QStringLiteral("title"), defaultTitle).toString()
            : value.isValid() ? value.toString()
                              : defaultTitle;
        QString alignment =
            descriptor.value(QStringLiteral("alignment"), descriptor.value(QStringLiteral("align")))
                .toString();
        if (alignment.isEmpty())
            alignment = alignmentName(modelHeader(column, Qt::TextAlignmentRole));
        if (alignment.isEmpty()) alignment = QStringLiteral("left");
        int leftPadding = 0;
        int rightPadding = 0;
        if (descriptor.contains(QStringLiteral("padding"))) {
            leftPadding = rightPadding = descriptor.value(QStringLiteral("padding")).toInt();
        }
        if (descriptor.contains(QStringLiteral("leftPadding"))) {
            leftPadding = descriptor.value(QStringLiteral("leftPadding")).toInt();
        }
        if (descriptor.contains(QStringLiteral("rightPadding"))) {
            rightPadding = descriptor.value(QStringLiteral("rightPadding")).toInt();
        }
        const auto columnStyle = [&](const char* key) {
            const QVariant entry = descriptor.value(QLatin1String(key));
            QJSValue callable;
            if (entry.metaType() == QMetaType::fromType<QJSValue>()) {
                const QJSValue value = entry.value<QJSValue>();
                if (value.isCallable()) callable = value;
            }
            const QString text = callable.isCallable() ? QString() : entry.toString();
            return std::make_tuple(text.isEmpty() ? 0u : Color::resolve(text, 0u), !text.isEmpty(),
                                   callable);
        };
        const auto [columnFg, hasColumnFg, columnFgFn] = columnStyle("color");
        const auto [columnBg, hasColumnBg, columnBgFn] = columnStyle("background");
        m_geometry.append({column, int(x), columnWidth, title, alignment, leftPadding, rightPadding,
                           descriptor.value(QStringLiteral("headerAlign"), alignment).toString(),
                           columnFg, columnBg, hasColumnFg, hasColumnBg, columnFgFn, columnBgFn});
        if (overscan) break;
        x += columnWidth;
        if (x >= view.right()) {
            if (x > (std::numeric_limits<int>::max)()) break;
            overscan = true;
        }
    }
}
QVariant TableView::cellRawValue(int row, int col) const {
    if (m_cppModel) {
        const QModelIndex idx = sourceIndex(row, col);
        return m_cppModel->data(idx, Qt::DisplayRole);
    }
    if (row >= 0 && row < m_listModel.size()) {
        QVariant r = m_listModel[row];
        if (r.userType() == QMetaType::fromType<QJSValue>().id()) {
            r = r.value<QJSValue>().toVariant();
        }
        if (r.canConvert<QVariantList>()) {
            const QVariantList rowList = r.toList();
            if (col >= 0 && col < rowList.size()) return unpack(rowList[col]);
        } else if (r.canConvert<QVariantMap>()) {
            const QVariantMap rowMap = r.toMap();
            if (col >= 0 && col < m_columns.size()) {
                const QVariant colVal = m_columns[col];
                QString key;
                if (colVal.canConvert<QVariantMap>()) {
                    const QVariantMap colMap = colVal.toMap();
                    key = colMap.value("key", colMap.value("title")).toString();
                } else {
                    key = colVal.toString();
                }
                return unpack(rowMap.value(key));
            }
        }
    }
    return {};
}
QString TableView::cellText(int row, int col) const {
    const QVariant raw = cellRawValue(row, col);
    if (!raw.isValid()) return {};
    if (raw.metaType() == QMetaType::fromType<QString>()) return raw.toString();
    if (raw.metaType() == QMetaType::fromType<Span>()) return raw.value<Span>().text();
    QList<Span> spans;
    if (Span::tryExtractList(raw, /*allowVariantMap=*/false, spans)) return Span::plainText(spans);
    return raw.toString();
}

void TableView::handleDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                  const QList<int>& roles) {
    m_fetchedRoles.clear();
    if (!topLeft.isValid() || !bottomRight.isValid() || topLeft.parent().isValid()) {
        m_builtInCells.clear();
    } else if (!m_builtInCells.isEmpty()) {
        for (auto it = m_builtInCells.begin(); it != m_builtInCells.end();) {
            if (it.key().first >= topLeft.row() && it.key().first <= bottomRight.row())
                it = m_builtInCells.erase(it);
            else
                ++it;
        }
    }
    bool affected = false;
    for (auto& record : m_createdDelegateItems) {
        const QModelIndex index = record.source;
        const bool columnMatches =
            m_rowDelegate
                ? (bottomRight.column() >= 0 && topLeft.column() < columnCount())
                : (index.column() >= topLeft.column() && index.column() <= bottomRight.column());
        if (!topLeft.isValid() || !bottomRight.isValid() ||
            (index.isValid() && index.parent() == topLeft.parent() &&
             index.row() >= topLeft.row() && index.row() <= bottomRight.row() && columnMatches)) {
            if (roles.isEmpty())
                record.allRolesDirty = true;
            else
                for (int role : roles)
                    record.dirtyRoles.insert(role);
            affected = true;
        }
    }
    if (affected)
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    else if (!m_delegate && !m_rowDelegate)
        markDirty();
}
void TableView::updateDelegateState(DelegateRecord& record, int row, int column) {
    const bool current = row == m_selectedRow && column == m_currentColumn;
    if (!record.item || record.current == current) return;
    record.current = current;
    QQmlContext* context = delegateContext(record.item);
    for (const char* name : {"current", "selected"}) {
        DelegateUtils::setContextProperty(record.component, context, QString::fromLatin1(name),
                                          current);
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
void TableView::updateDelegate(DelegateRecord& record, int row, int column) {
    const bool cellMode = bool(m_delegate);
    const bool dataDirty = !record.loaded || record.allRolesDirty || !record.dirtyRoles.isEmpty();
    const bool allRolesDirty = record.allRolesDirty;
    const QSet<int> dirtyRoles = std::exchange(record.dirtyRoles, {});
    record.allRolesDirty = false;
    const bool moved = record.row != row || record.column != column;
    if (!dataDirty && !moved && record.item) {
        updateDelegateState(record, row, column);
        return;
    }
    QVariantMap values = record.values;
    QVariant modelData = record.modelData;
    if (dataDirty) {
        if (m_cppModel) {
            auto& fetched = m_fetchedRoles[record.source];
            const auto readRole = [&](int role) {
                if (!fetched.contains(role))
                    fetched.insert(role, m_cppModel->data(record.source, role));
                return fetched.value(role);
            };
            if (allRolesDirty || !record.loaded) values.clear();
            for (auto it = m_roleNameStrings.cbegin(); it != m_roleNameStrings.cend(); ++it)
                if (allRolesDirty || !record.loaded || dirtyRoles.contains(it.key()))
                    values.insert(it.value(), readRole(it.key()));
            NumericItemRoles::apply(m_cppModel.data(), record.source, values, true, m_claimedNames);
            modelData = m_roleNames.isEmpty() ? readRole(Qt::DisplayRole) : QVariant(values);
        } else {
            const QVariant value =
                row >= 0 && row < m_listModel.size() ? unpack(m_listModel[row]) : QVariant();
            values = value.toMap();
            modelData = cellMode ? QVariant(cellText(row, column)) : value;
            if (cellMode) values.insert(QStringLiteral("display"), modelData);
        }
    }
    QQmlComponent* raw = m_delegate ? m_delegate.data() : m_rowDelegate.data();
    QQmlComponent* component = record.component;
    if (dataDirty || moved || !record.item)
        component = DelegateUtils::resolveDelegate(raw, row, column, values);
    if (record.component != component) destroyDelegate(record);
    const auto assign = [&](const QString& name, const QVariant& value) {
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
        if (QQmlContext* context = DelegateUtils::createContext(component, this)) {
            for (auto it = initial.cbegin(); it != initial.cend(); ++it)
                DelegateUtils::setContextProperty(component, context, it.key(), it.value());
            record.container = new Item(this);
            record.container->setParentItem(this);
            record.container->setClip(true);
            record.item =
                DelegateUtils::createItem(component, initial, context, record.container, this);
            if (!record.item) {
                delete record.container.data();
                record.container.clear();
            }
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
            assign(QStringLiteral("index"), index);
            assign(QStringLiteral("row"), row);
            if (cellMode) assign(QStringLiteral("column"), column);
        } else if (record.item->property("index").toLongLong() != index) {
            assign(QStringLiteral("index"), index);
        }
        updateDelegateState(record, row, column);
    }
    record.row = row;
    record.column = column;
    record.values = values;
    record.modelData = modelData;
    record.loaded = true;
}
TableView::ViewGeometry TableView::viewGeometry() const {
    const int inset = m_showOuterBorder ? 1 : 0;
    const int innerWidth = (std::max)(0, width() - 2 * inset);
    const int innerHeight = (std::max)(0, height() - 2 * inset);
    const int header = (std::min)(headerRows(), innerHeight);
    const int body = innerHeight - header;
    const int capacity = m_showRowGrid ? body / 2 + body % 2 : body;
    return {inset,
            inset,
            innerWidth,
            innerHeight,
            inset + header,
            body,
            m_showRowGrid ? 2 : 1,
            innerWidth > 0 ? capacity : 0};
}

int TableView::computeFirstVisibleRow() const {
    const int visibleRows = viewGeometry().capacity;
    if (visibleRows == 0) return 0;
    const int maxFirst = (std::max)(0, rowCount() - visibleRows);

    // The viewport is remembered, not derived from the selection. Deriving it
    // pinned the selected row to the last visible line the moment you scrolled
    // past one page, so every Up press dragged the whole view instead of
    // moving the cursor inside it. Same rule as TreeView::adjustScrollTop():
    // move only when the selection would fall outside the window.
    int first = std::clamp(m_firstVisibleRow, 0, maxFirst);
    if (m_selectedRow >= 0) {
        if (m_selectedRow < first) {
            first = m_selectedRow;
        } else if (m_selectedRow >= first + visibleRows) {
            first = m_selectedRow - visibleRows + 1;
        }
    }
    return std::clamp(first, 0, maxFirst);
}
void TableView::synchronizeViewport() {
    // Rows removed from or inserted above the viewport renumber everything
    // after them. Qt shifts a persistent index across that, so the anchor
    // keeps the same *content* on the top line instead of the same row
    // number. Without it a trace ring evicting from the front scrolls the
    // view forward on every batch.
    if (!m_cppModel) return;
    // Sticky at the top. A view that has not been scrolled should reveal rows
    // inserted above it, not be pushed down past them -- holding content
    // still is only what you want once the user has chosen a position.
    if (m_firstVisibleRow > 0 && m_topIndex.isValid() && !m_topIndex.parent().isValid()) {
        m_firstVisibleRow = m_topIndex.row();
    }
    m_firstVisibleRow =
        std::clamp(m_firstVisibleRow, 0, (std::max)(0, rowCount() - viewGeometry().capacity));
}
void TableView::updateLayout() {
    Item::updateLayout();
    resolveColumns();
    m_firstVisibleRow = computeFirstVisibleRow();
    m_topIndex = m_cppModel ? QPersistentModelIndex(sourceIndex(m_firstVisibleRow, 0))
                            : QPersistentModelIndex();
    layoutDelegates();
    maybeFetchMore();
}
void TableView::maybeFetchMore() {
    if (!m_cppModel || viewGeometry().capacity == 0) return;
    // Rows the current layout shows or overscans, in both delegate and
    // built-in painting modes.
    const auto lastShown = [this] {
        return int((std::min)(qint64(rowCount()) - 1,
                              qint64(m_firstVisibleRow) + viewGeometry().capacity + 1));
    };
    DelegateUtils::fetchMoreIfNeeded(m_cppModel.data(), QModelIndex(), 2, m_fetching, lastShown);
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
            auto& record = it.value();
            if (!record.source.isValid() || record.source.parent().isValid()) {
                destroyDelegate(record);
                continue;
            }
            const int column = m_delegate && m_cppModel->columnCount() > 1 ? record.source.column()
                                                                           : it.key().second;
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
    const auto view = viewGeometry();
    const int bodyHeight = view.capacity;
    if (bodyHeight > 0) {
        const int first = (std::max)(0, m_firstVisibleRow - 2);
        const int last =
            int((std::min)(qint64(rowCount()) - 1, qint64(m_firstVisibleRow) + bodyHeight + 1));
        for (int row = first; row <= last; ++row) {
            if (m_delegate) {
                for (const auto& geometry : std::as_const(m_geometry))
                    wanted.insert({row, geometry.column}, geometry);
            } else
                wanted.insert({row, 0}, {0, view.left, view.width, {}, {}});
        }
    }
    QVarLengthArray<DelegateRecord, 32> recyclePool;
    for (auto it = m_createdDelegateItems.begin(); it != m_createdDelegateItems.end();) {
        if (!wanted.contains(it.key())) {
            recyclePool.append(std::move(it.value()));
            it = m_createdDelegateItems.erase(it);
        } else
            ++it;
    }
    m_fetchedRoles.clear();
    QQmlComponent* raw = m_delegate ? m_delegate.data() : m_rowDelegate.data();
    const bool isChooser = raw && QQmlListReference(raw, "choices").isValid();
    for (auto it = wanted.cbegin(); it != wanted.cend(); ++it) {
        const int row = it.key().first;
        const int column = it.key().second;
        if (!m_createdDelegateItems.contains(it.key()) && !isChooser && !recyclePool.isEmpty()) {
            for (qsizetype i = 0; i < recyclePool.size(); ++i) {
                if (recyclePool[i].item && recyclePool[i].component == raw) {
                    DelegateRecord recycled = std::move(recyclePool[i]);
                    recyclePool.remove(i);
                    recycled.allRolesDirty = true;
                    m_createdDelegateItems.insert(it.key(), std::move(recycled));
                    break;
                }
            }
        }
        auto& record = m_createdDelegateItems[it.key()];
        const QModelIndex source = sourceIndex(row, m_delegate ? column : 0);
        if (record.source != source) {
            record.source = source;
            record.allRolesDirty = true;
        }
        updateDelegate(record, row, column);
        if (record.item) {
            const auto& geometry = it.value();
            const bool visible = row >= m_firstVisibleRow &&
                                 qint64(row) < qint64(m_firstVisibleRow) + bodyHeight &&
                                 geometry.x < view.right();
            // Hidden overscan items need no potentially huge absolute origin.
            record.container->setX(visible ? geometry.x : 0);
            record.container->setY(visible ? view.bodyTop + (row - m_firstVisibleRow) * view.stride
                                           : 0);
            record.container->setWidth(
                (std::max)(0, (std::min)(geometry.width, view.right() - geometry.x)));
            record.container->setHeight(1);
            record.container->setVisible(visible);
            record.item->setX(0);
            record.item->setY(0);
            record.item->setWidth(geometry.width);
            record.item->setHeight(1);
            record.item->setVisible(visible);
        }
    }
    for (auto& record : recyclePool)
        destroyDelegate(record);
    m_fetchedRoles.clear();
}

const TableView::BuiltInCell& TableView::resolveBuiltInCell(int row, int column) const {
    const Position key{row, column};
    const auto existing = m_builtInCells.constFind(key);
    if (existing != m_builtInCells.cend()) return existing.value();

    BuiltInCell entry;
    entry.index = sourceIndex(row, column);
    const QVariant rawCell =
        m_cppModel ? m_cppModel->data(entry.index, Qt::DisplayRole) : cellRawValue(row, column);
    if (rawCell.metaType() == QMetaType::fromType<QString>()) {
        entry.rawValue = rawCell.toString();
    } else if (rawCell.metaType() == QMetaType::fromType<Span>()) {
        const Span s = rawCell.value<Span>();
        entry.rawValue = s.text();
        entry.spans = {s};
    } else if (Span::tryExtractList(rawCell, /*allowVariantMap=*/false, entry.spans)) {
        entry.rawValue = Span::plainText(entry.spans);
    } else {
        entry.rawValue = rawCell.toString();
    }
    entry.text = entry.rawValue;

    if (m_cppModel && entry.index.isValid()) {
        entry.standardAlignment =
            NumericItemRoles::alignmentName(m_cppModel->data(entry.index, Qt::TextAlignmentRole));
        const QVariant check = m_cppModel->data(entry.index, Qt::CheckStateRole);
        const QString glyph =
            NumericItemRoles::decorationGlyph(m_cppModel->data(entry.index, Qt::DecorationRole));
        QString prefix;
        if (!glyph.isEmpty()) prefix = glyph + QLatin1Char(' ');
        if (check.isValid() && check.canConvert<int>() && check.toInt() >= 0) {
            const int state = check.toInt();
            prefix += state == Qt::Checked            ? QStringLiteral("[x] ")
                      : state == Qt::PartiallyChecked ? QStringLiteral("[-] ")
                                                      : QStringLiteral("[ ] ");
        }
        if (!prefix.isEmpty()) {
            entry.text.prepend(prefix);
            if (!entry.spans.isEmpty()) entry.spans.prepend(Span(prefix));
        }
        entry.enabled = m_cppModel->flags(entry.index).testFlag(Qt::ItemIsEnabled);
    }

    if (m_cellFormat.isCallable()) {
        const QJSValue result = m_cellFormat.call({row, column, entry.text});
        if (result.isError()) {
            callbackWarning(QStringLiteral("cellFormat: ") + result.toString() + QLatin1Char('\n') +
                                result.property("stack").toString(),
                            m_formatErrors);
        } else {
            m_formatErrors.clear();
            if (result.isArray()) {
                QList<Span> formattedSpans;
                if (Span::tryExtractList(result.toVariant(), /*allowVariantMap=*/true,
                                         formattedSpans)) {
                    entry.spans = std::move(formattedSpans);
                    entry.spansOverrideSelection = true;
                }
            } else if (result.isObject()) {
                const QVariant retained = result.toVariant(QJSValue::RetainJSObjects);
                if (retained.metaType() == QMetaType::fromType<Span>()) {
                    const Span s = retained.value<Span>();
                    entry.text = s.text();
                    if (s.color().isValid() && !s.color().isTransparent())
                        entry.formatFg = s.color();
                    if (s.backgroundColor().isValid() && !s.backgroundColor().isTransparent())
                        entry.formatBg = s.backgroundColor();
                    entry.formatStyle = s.style();
                    entry.spans.clear();
                } else {
                    const QJSValue textProp = result.property(QStringLiteral("text"));
                    if (!textProp.isUndefined()) {
                        entry.text = textProp.toString();
                        entry.spans.clear();
                    }
                    const QJSValue spansProp = result.property(QStringLiteral("spans"));
                    if (!spansProp.isUndefined()) {
                        QList<Span> formattedSpans;
                        if (Span::tryExtractList(spansProp.toVariant(), /*allowVariantMap=*/true,
                                                 formattedSpans)) {
                            entry.spans = std::move(formattedSpans);
                            entry.spansOverrideSelection = true;
                        }
                    }
                    const auto parseColorProp = [&](const QJSValue& prop) -> Color {
                        if (prop.isUndefined()) return {};
                        if (prop.isString()) {
                            const QString str = prop.toString();
                            if (str.isEmpty()) return {};
                            const auto cached = m_columnColorCache.constFind(str);
                            if (cached != m_columnColorCache.cend())
                                return Color::fromArgb32(cached.value());
                            const Color parsed = Color::fromString(str);
                            if (!parsed.isValid()) return {};
                            if (m_columnColorCache.size() >= 256) m_columnColorCache.clear();
                            m_columnColorCache.insert(str, parsed.argb32());
                            return parsed;
                        }
                        const QVariant cv = prop.toVariant(QJSValue::RetainJSObjects);
                        if (cv.metaType() == QMetaType::fromType<Color>()) return cv.value<Color>();
                        return Color::fromString(prop.toString());
                    };
                    entry.formatFg = parseColorProp(result.property(QStringLiteral("color")));
                    entry.formatBg =
                        parseColorProp(result.property(QStringLiteral("backgroundColor")));
                    const bool formatGaveFg = entry.formatFg.isValid();
                    const bool formatGaveBg = entry.formatBg.isValid();
                    const auto applyFlag = [&](const QString& flagKey,
                                               void (TextStyle::*setter)(bool)) {
                        const QJSValue p = result.property(flagKey);
                        if (!p.isUndefined()) (entry.formatStyle.*setter)(p.toBool());
                    };
                    applyFlag(QStringLiteral("bold"), &TextStyle::setBold);
                    applyFlag(QStringLiteral("dim"), &TextStyle::setDim);
                    applyFlag(QStringLiteral("italic"), &TextStyle::setItalic);
                    applyFlag(QStringLiteral("underline"), &TextStyle::setUnderline);
                    applyFlag(QStringLiteral("reversed"), &TextStyle::setReversed);
                    if (!entry.spansOverrideSelection && !entry.spans.isEmpty()) {
                        for (Span& s : entry.spans) {
                            if (formatGaveFg) s.resetColor();
                            if (formatGaveBg) s.resetBackgroundColor();
                            if (!entry.formatStyle.isEmpty()) {
                                TextStyle st = s.style();
                                st.applyOverlay(entry.formatStyle);
                                s.setStyle(st);
                            }
                        }
                    }
                    const QJSValue alignProp = result.property(QStringLiteral("align"));
                    if (!alignProp.isUndefined()) entry.formatAlignment = alignProp.toString();
                    const QJSValue padProp = result.property(QStringLiteral("padding"));
                    if (!padProp.isUndefined()) {
                        entry.leftPadding = entry.rightPadding = (std::max)(0, padProp.toInt());
                    }
                    const QJSValue lPadProp = result.property(QStringLiteral("leftPadding"));
                    if (!lPadProp.isUndefined()) {
                        entry.leftPadding = (std::max)(0, lPadProp.toInt());
                    }
                    const QJSValue rPadProp = result.property(QStringLiteral("rightPadding"));
                    if (!rPadProp.isUndefined()) {
                        entry.rightPadding = (std::max)(0, rPadProp.toInt());
                    }
                }
            }
        }
    }

    if (entry.spans.isEmpty()) {
        entry.totalWidth = UnicodeText::displayWidth(entry.text);
    } else {
        entry.spanWidths.reserve(entry.spans.size());
        int sum = 0;
        for (const Span& s : std::as_const(entry.spans)) {
            const int w = UnicodeText::displayWidth(s.text());
            entry.spanWidths.append(w);
            sum += w;
        }
        entry.totalWidth = sum;
    }

    if (m_builtInCells.size() >= 1024) m_builtInCells.clear();
    return m_builtInCells.insert(key, std::move(entry)).value();
}

void TableView::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible) return;
    const auto view = viewGeometry();
    const ClipRect clip =
        clipRect.intersected({0, 0, buffer.width(), buffer.height()})
            .intersected({absX + view.left, absY + view.top, view.width, view.height});
    if (clip.width == 0 || clip.height == 0) return;
    const auto write = [&](int x, int y, const QString& text, uint32_t fg, uint32_t bg,
                           TextAttribute attributes) {
        if (y < clip.y || y >= clip.y + clip.height) return;
        // alignedPrefix already clips the right edge; only a clipped left edge
        // needs grapheme-by-grapheme handling.
        if (x >= clip.x) {
            buffer.writeText(x, y, text, fg, bg, attributes);
            return;
        }
        GraphemeIterator it(text);
        GraphemeRef cluster;
        while (it.next(cluster)) {
            if (x >= clip.x + clip.width) break;
            if (clip.contains(x, y) && clip.contains(x + (std::max)(1, cluster.width) - 1, y))
                buffer.writeText(x, y, QString(cluster.utf16), fg, bg, attributes);
            x += cluster.width;
        }
    };
    const auto fillSpaces = [&](int x, int y, int count, uint32_t spaceFg, uint32_t spaceBg,
                                TextAttribute spaceAttributes) {
        if (count <= 0 || y < clip.y || y >= clip.y + clip.height) return;
        const int begin = (std::max)(clip.x, x);
        const int end = (std::min)(clip.x + clip.width, x + count);
        if (end <= begin) return;
        if (begin > 0 && buffer.cellAt(begin, y).continuation) {
            Cell& prev = buffer.cellAt(begin - 1, y);
            prev.setCharacter(" ");
            prev.displayWidth = 1;
            prev.continuation = false;
        }
        if (end < buffer.width() && buffer.cellAt(end - 1, y).displayWidth == 2) {
            Cell& next = buffer.cellAt(end, y);
            next.setCharacter(" ");
            next.displayWidth = 1;
            next.continuation = false;
        }
        buffer.fillRect(begin, y, end - begin, 1, " ", spaceFg, spaceBg);
        for (int cx = begin; cx < end; ++cx) {
            Cell& c = buffer.cellAt(cx, y);
            c.attributes = spaceAttributes;
            c.linkId = 0;
        }
    };
    const auto visibleWidth = [&](const ColumnGeometry& geometry) {
        return (std::max)(0, (std::min)(geometry.width, clip.x + clip.width - absX - geometry.x));
    };
    const auto paintSpans = [&](const QList<Span>& spans, const QList<int>& spanWidths,
                                int totalSpanWidth, int x, int y, int width, int visWidth,
                                const QString& alignment, int leftPadding, int rightPadding,
                                uint32_t baseFg, uint32_t baseBg, TextAttribute baseAttributes,
                                bool useSpanColors) {
        if (width <= 0 || visWidth <= 0 || y < clip.y || y >= clip.y + clip.height) return;
        int leftPad = (std::max)(0, leftPadding);
        int rightPad = (std::max)(0, rightPadding);
        if (leftPad > width) leftPad = width;
        if (rightPad > width - leftPad) rightPad = width - leftPad;
        const int innerWidth = width - leftPad - rightPad;

        int used = totalSpanWidth;
        if (used > innerWidth) {
            used = 0;
            for (const Span& span : spans) {
                GraphemeIterator measure(span.text());
                GraphemeRef cluster;
                bool full = false;
                while (measure.next(cluster)) {
                    if (cluster.width > innerWidth - used) {
                        full = true;
                        break;
                    }
                    used += cluster.width;
                }
                if (full) break;
            }
        }
        const int extraSpace = innerWidth - used;
        const int alignOffset = alignment == QLatin1String("right")    ? extraSpace
                                : alignment == QLatin1String("center") ? extraSpace / 2
                                                                       : 0;
        const int left = leftPad + alignOffset;
        const int leadSpaces = (std::min)(left, visWidth);
        if (leadSpaces > 0) {
            fillSpaces(x, y, leadSpaces, baseFg, baseBg, baseAttributes);
        }

        int position = left;
        const int maxContentPos = (std::min)(width - rightPad, visWidth);
        if (totalSpanWidth <= innerWidth && left + totalSpanWidth <= maxContentPos) {
            for (qsizetype i = 0; i < spans.size(); ++i) {
                const Span& span = spans[i];
                const int spanW = spanWidths[i];
                if (span.text().isEmpty()) continue;
                const uint32_t spanFg = useSpanColors ? span.resolveForeground(baseFg) : baseFg;
                const uint32_t spanBg = useSpanColors ? span.resolveBackground(baseBg) : baseBg;
                const TextAttribute spanAttr = span.resolveAttributes(baseAttributes);
                write(x + position, y, span.text(), spanFg, spanBg, spanAttr);
                position += spanW;
            }
        } else {
            for (const Span& span : spans) {
                if (position >= maxContentPos) break;
                const uint32_t spanFg = useSpanColors ? span.resolveForeground(baseFg) : baseFg;
                const uint32_t spanBg = useSpanColors ? span.resolveBackground(baseBg) : baseBg;
                const TextAttribute spanAttr = span.resolveAttributes(baseAttributes);
                QString runText;
                const int runStart = position;
                GraphemeIterator draw(span.text());
                GraphemeRef cluster;
                while (position < maxContentPos && draw.next(cluster)) {
                    if (cluster.width > maxContentPos - position) break;
                    runText.append(cluster.utf16);
                    position += cluster.width;
                }
                if (!runText.isEmpty()) {
                    write(x + runStart, y, runText, spanFg, spanBg, spanAttr);
                }
            }
        }
        if (position < visWidth) {
            fillSpaces(x + position, y, visWidth - position, baseFg, baseBg, baseAttributes);
        }
    };
    if (headerRows()) {
        const uint32_t headerBg = m_resolvedHeaderBackgroundColor;
        const int headerY = absY + view.top;
        if (headerBg != 0 && headerY >= clip.y && headerY < clip.y + clip.height) {
            const int begin = (std::max)(clip.x, absX + view.left);
            const int end = (std::min)(clip.x + clip.width, absX + view.right());
            if (end > begin) {
                buffer.fillRect(begin, headerY, end - begin, 1, " ", m_resolvedHeaderColor,
                                headerBg);
                for (int x = begin; x < end; ++x)
                    if (Cell* cell = buffer.tryCellAt(x, headerY))
                        cell->attributes = TextAttribute::Bold;
            }
        }
        for (const auto& geometry : std::as_const(m_geometry)) {
            if (geometry.x >= view.right()) break;
            write(absX + geometry.x, absY + view.top,
                  alignedPrefix(geometry.title, geometry.width, visibleWidth(geometry),
                                geometry.headerAlignment, geometry.leftPadding,
                                geometry.rightPadding),
                  m_resolvedHeaderColor, m_resolvedHeaderBackgroundColor, TextAttribute::Bold);
        }
    }
    for (int r = 0; r < view.capacity && qint64(m_firstVisibleRow) + r < rowCount(); ++r) {
        const int row = m_firstVisibleRow + r;
        const int y = absY + view.bodyTop + r * view.stride;
        if (y < clip.y || y >= clip.y + clip.height) continue;
        const bool selected = row == m_selectedRow;
        const uint32_t bg = selected                       ? m_resolvedSelectionBackgroundColor
                            : m_alternatingRows && row % 2 ? m_resolvedAltRowBackgroundColor
                                                           : m_resolvedRowBackgroundColor;
        const uint32_t fg =
            selected && m_selectionColor.isValid() ? m_resolvedSelectionColor : m_resolvedTextColor;
        const TextAttribute attributes =
            (selected ? TextAttribute::Bold : TextAttribute::None) |
            (selected && !m_selectionColor.isValid() && !m_selectionBackgroundColor.isValid()
                 ? TextAttribute::Reversed
                 : TextAttribute::None);
        if (selected || bg != 0) {
            const int begin = (std::max)(clip.x, absX + view.left);
            const int end = (std::min)(clip.x + clip.width, absX + view.right());
            if (end > begin) {
                if ((bg >> 24) != 0) {
                    buffer.fillRect(begin, y, end - begin, 1, " ", fg, bg);
                    if (attributes != TextAttribute::None) {
                        for (int x = begin; x < end; ++x)
                            if (Cell* cell = buffer.tryCellAt(x, y)) cell->attributes = attributes;
                    }
                } else {
                    for (int x = begin; x < end; ++x)
                        buffer.setGrapheme(x, y, QByteArrayView(" ", 1), 1, fg, bg, attributes);
                }
            }
        }
        if (m_delegate || m_rowDelegate) continue;
        for (const auto& geometry : std::as_const(m_geometry)) {
            if (geometry.x >= view.right()) break;
            const BuiltInCell& cell = resolveBuiltInCell(row, geometry.column);
            const QString& alignment = !cell.formatAlignment.isEmpty()     ? cell.formatAlignment
                                       : !cell.standardAlignment.isEmpty() ? cell.standardAlignment
                                                                           : geometry.alignment;
            const int cellLeftPadding =
                cell.leftPadding >= 0 ? cell.leftPadding : geometry.leftPadding;
            const int cellRightPadding =
                cell.rightPadding >= 0 ? cell.rightPadding : geometry.rightPadding;
            uint32_t cellFg = fg;
            uint32_t cellBg = bg;
            TextAttribute cellAttributes = attributes;
            // A column that declares its own ink wins over the model's
            // Qt::ForegroundRole: the role is the compatibility path for models
            // ported from widgets, and an explicit view-side rule outranks it.
            const bool keepCellColors = !selected || m_selectionKeepsCellColors;
            // Whether the column actually supplied a colour for this cell, as
            // opposed to merely offering a function: a function that returns
            // nothing defers to the model's role.
            bool columnGaveFg = false;
            bool columnGaveBg = false;
            if (keepCellColors) {
                if (geometry.hasForeground) {
                    cellFg = geometry.foreground;
                    columnGaveFg = true;
                } else if (const auto ink = columnColor(geometry.foregroundFn, row, geometry.column,
                                                        cell.rawValue)) {
                    cellFg = *ink;
                    columnGaveFg = true;
                }
                if (geometry.hasBackground) {
                    cellBg = geometry.background;
                    columnGaveBg = true;
                } else if (const auto fill = columnColor(geometry.backgroundFn, row,
                                                         geometry.column, cell.rawValue)) {
                    cellBg = *fill;
                    columnGaveBg = true;
                }
            }
            if (m_cppModel && cell.index.isValid()) {
                if (keepCellColors && !columnGaveFg) {
                    const QString text = NumericItemRoles::colorString(
                        m_cppModel->data(cell.index, Qt::ForegroundRole));
                    if (!text.isEmpty()) cellFg = resolveCachedColor(text, cellFg);
                }
                if (keepCellColors && !columnGaveBg) {
                    const QString text = NumericItemRoles::colorString(
                        m_cppModel->data(cell.index, Qt::BackgroundRole));
                    if (!text.isEmpty()) cellBg = resolveCachedColor(text, cellBg);
                }
                if (!cell.enabled) cellAttributes = cellAttributes | TextAttribute::Dim;
            }
            if (cell.formatFg.isValid()) cellFg = cell.formatFg.argb32();
            if (cell.formatBg.isValid()) cellBg = cell.formatBg.argb32();
            cellAttributes = cell.formatStyle.resolveAttributes(cellAttributes);

            const int visW = visibleWidth(geometry);
            if (cell.spans.isEmpty()) {
                const int width = geometry.width;
                if (width <= 0 || visW <= 0) continue;
                int leftPad = (std::max)(0, cellLeftPadding);
                int rightPad = (std::max)(0, cellRightPadding);
                if (leftPad > width) leftPad = width;
                if (rightPad > width - leftPad) rightPad = width - leftPad;
                const int innerWidth = width - leftPad - rightPad;
                const int maxContentPos = (std::min)(width - rightPad, visW);
                const int extraSpace = innerWidth - cell.totalWidth;
                const int alignOffset = alignment == QLatin1String("right")    ? extraSpace
                                        : alignment == QLatin1String("center") ? extraSpace / 2
                                                                               : 0;
                const int left = leftPad + alignOffset;
                if (cell.totalWidth <= innerWidth && left + cell.totalWidth <= maxContentPos &&
                    (cell.totalWidth > 0 || cell.text.isEmpty())) {
                    const int leadSpaces = (std::min)(left, visW);
                    if (leadSpaces > 0) {
                        fillSpaces(absX + geometry.x, y, leadSpaces, cellFg, cellBg,
                                   cellAttributes);
                    }
                    if (!cell.text.isEmpty()) {
                        write(absX + geometry.x + left, y, cell.text, cellFg, cellBg,
                              cellAttributes);
                    }
                    const int position = (std::max)(leadSpaces, left + cell.totalWidth);
                    if (position < visW) {
                        fillSpaces(absX + geometry.x + position, y, visW - position, cellFg, cellBg,
                                   cellAttributes);
                    }
                } else {
                    write(absX + geometry.x, y,
                          alignedPrefix(cell.text, geometry.width, visW, alignment, cellLeftPadding,
                                        cellRightPadding),
                          cellFg, cellBg, cellAttributes);
                }
            } else {
                paintSpans(cell.spans, cell.spanWidths, cell.totalWidth, absX + geometry.x, y,
                           geometry.width, visW, alignment, cellLeftPadding, cellRightPadding,
                           cellFg, cellBg, cellAttributes,
                           keepCellColors || cell.spansOverrideSelection);
            }
        }
    }
}

void TableView::paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                             const ClipRect& clipRect) const {
    if (!m_visible || width() <= 0 || height() <= 0) return;
    // Order: horizontal, vertical, corners, top/bottom/left/right tees, cross.
    const QString glyphs = (m_gridStyle == Double)  ? QStringLiteral("═║╔╗╚╝╦╩╠╣╬")
                           : (m_gridStyle == Ascii) ? QStringLiteral("-|+++++++++")
                                                    : QStringLiteral("─│┌┐└┘┬┴├┤┼");
    const ClipRect clip = clipRect.intersected({0, 0, buffer.width(), buffer.height()})
                              .intersected({absX, absY, width(), height()});
    if (clip.width == 0 || clip.height == 0) return;
    const uint32_t gridFg = m_gridColor.isValid() ? m_resolvedGridColor : m_resolvedTextColor;
    const auto put = [&](int x, int y, GridGlyph glyph) {
        if (clip.contains(absX + x, absY + y)) {
            const int cellX = absX + x;
            const int cellY = absY + y;
            int lead = cellX;
            while (lead > clip.x && buffer.cellAt(lead, cellY).continuation)
                --lead;
            const int end = (std::min)(clip.x + clip.width,
                                       lead + int(buffer.cellAt(lead, cellY).displayWidth));
            for (int part = lead; part < end; ++part)
                if (part != cellX) buffer.writeText(part, cellY, QStringLiteral(" "), gridFg);
            buffer.writeText(cellX, cellY, QString(glyphs[glyph]), gridFg);
        }
    };
    const auto horizontal = [&](int y, int left, int right, GridGlyph junction) {
        const int begin = (std::max)(left, clip.x - absX);
        const int end = (std::min)(right, clip.x + clip.width - absX);
        for (int x = begin; x < end; ++x)
            put(x, y, Horizontal);
        for (int x : m_separators)
            put(x, y, junction);
    };
    if (m_showOuterBorder && (width() == 1 || height() == 1)) {
        if (width() == 1 && height() == 1)
            put(0, 0, Cross);
        else if (height() == 1)
            horizontal(0, 0, width(), Horizontal);
        else
            for (int y = (std::max)(0, clip.y - absY);
                 y < (std::min)(height(), clip.y + clip.height - absY); ++y)
                put(0, y, Vertical);
        return;
    }
    const auto view = viewGeometry();
    const auto verticals = [&](int y) {
        for (int x : m_separators)
            put(x, y, Vertical);
    };
    if (m_showOuterBorder) {
        horizontal(0, view.left, view.right(), TopTee);
        horizontal(height() - 1, view.left, view.right(), BottomTee);
        put(0, 0, TopLeft);
        put(width() - 1, 0, TopRight);
        put(0, height() - 1, BottomLeft);
        put(width() - 1, height() - 1, BottomRight);
        for (int y = (std::max)(1, clip.y - absY);
             y < (std::min)(height() - 1, clip.y + clip.height - absY); ++y) {
            put(0, y, Vertical);
            put(width() - 1, y, Vertical);
        }
    }
    const auto divider = [&](int y) {
        horizontal(y, view.left, view.right(), Cross);
        if (m_showOuterBorder) {
            put(0, y, LeftTee);
            put(width() - 1, y, RightTee);
        }
    };
    if (headerRows() && view.height > 0) {
        verticals(view.top);
        if (view.height > 1) divider(view.top + 1);
    }
    if (m_showRowGrid) {
        for (int r = 0; r < view.capacity; ++r) {
            const int y = view.bodyTop + r * view.stride;
            if (y < view.bodyTop + view.bodyHeight) verticals(y);
            if (r + 1 < view.capacity) {
                const int divY = y + 1;
                if (divY < view.bodyTop + view.bodyHeight) divider(divY);
            }
        }
        // Dividers only sit between rows, so the last covered line is the last
        // row itself; any body lines after it still carry the column separators.
        const int lastCovered =
            view.bodyTop + (view.capacity > 0 ? (view.capacity - 1) * view.stride : -1);
        for (int y = lastCovered + 1; y < view.bodyTop + view.bodyHeight; ++y)
            verticals(y);
    } else {
        for (int y = view.bodyTop; y < view.bodyTop + view.bodyHeight; ++y)
            verticals(y);
    }
}

ClipRect TableView::clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const {
    const auto view = viewGeometry();
    return clipRect.intersected(
        {absX + view.left, absY + view.bodyTop, view.width, view.bodyHeight});
}

void TableView::keyPressEvent(KeyEvent* event) {
    const int page = (std::max)(1, viewGeometry().capacity);
    if (event->key() == Qt::Key_Up)
        setSelectedRow(m_selectedRow - 1);
    else if (event->key() == Qt::Key_Down)
        setSelectedRow(m_selectedRow + 1);
    else if (event->key() == Qt::Key_PageUp)
        setSelectedRow(m_selectedRow - page);
    else if (event->key() == Qt::Key_PageDown)
        setSelectedRow(int((std::min)(qint64(m_selectedRow) + page, qint64(rowCount()) - 1)));
    else if (event->key() == Qt::Key_Home) {
        if (rowCount() > 0) setSelectedRow(0);
    } else if (event->key() == Qt::Key_End) {
        if (rowCount() > 0) setSelectedRow(rowCount() - 1);
    } else if (event->key() == Qt::Key_Right)
        setCurrentColumn(m_currentColumn + 1);
    else if (event->key() == Qt::Key_Left)
        setCurrentColumn(m_currentColumn - 1);
    else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
             event->key() == Qt::Key_Space) {
        if (m_selectedRow >= 0 && m_selectedRow < rowCount()) emit rowActivated(m_selectedRow);
    } else
        return;
    event->setAccepted(true);
}
void TableView::mousePressEvent(MouseEvent* event) {
    const auto view = viewGeometry();
    if (event->button() != Qt::LeftButton || event->x() < view.left || event->x() >= view.right() ||
        event->y() < view.top || event->y() >= view.bottom())
        return;
    int column = -1;
    for (const auto& geometry : std::as_const(m_geometry))
        if (event->x() >= geometry.x && qint64(event->x()) < qint64(geometry.x) + geometry.width) {
            column = geometry.column;
            break;
        }
    if (column < 0) return;
    if (headerRows() && event->y() == view.top) {
        setFocus(true);
        emit headerClicked(column);
        event->setAccepted(true);
        return;
    }
    const int relativeY = event->y() - view.bodyTop;
    if (relativeY < 0 || relativeY % view.stride != 0) return;
    // m_firstVisibleRow, not a fresh compute: the two agreed while the
    // function was pure, and would not now that it carries state.
    const int row = m_firstVisibleRow + relativeY / view.stride;
    if (row >= 0 && row < rowCount()) {
        setFocus(true);
        setCurrentColumn(column);
        setSelectedRow(row);
        emit rowActivated(row);
        event->setAccepted(true);
    }
}
void TableView::wheelEvent(MouseEvent* event) {
    if (!event || event->wheelDeltaY() == 0) return;
    if ((focusPolicy() & (Qt::ClickFocus | Qt::WheelFocus)) && !focus()) setFocus(true);
    const int visibleRows = viewGeometry().capacity;
    const int maxFirst = (std::max)(0, rowCount() - visibleRows);
    const int nextFirst = std::clamp(m_firstVisibleRow - event->wheelDeltaY(), 0, maxFirst);
    if (nextFirst != m_firstVisibleRow) {
        m_firstVisibleRow = nextFirst;
        m_topIndex = m_cppModel ? QPersistentModelIndex(sourceIndex(m_firstVisibleRow, 0))
                                : QPersistentModelIndex();
        if (m_selectedRow >= 0 && visibleRows > 0) {
            const int clampedSelected =
                std::clamp(m_selectedRow, m_firstVisibleRow,
                           (std::min)(rowCount() - 1, m_firstVisibleRow + visibleRows - 1));
            if (clampedSelected != m_selectedRow) setSelectedRow(clampedSelected);
        }
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    }
    event->setAccepted(true);
}
} // namespace qmltui
