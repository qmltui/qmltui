#include <Qamui/Core/items/GridView.h>
#include <Qamui/Core/items/DelegateUtils.h>
#include <Qamui/Core/items/NumericItemRoles_p.h>
#include <QAbstractItemModel>
#include <QJSValue>
#include <QQmlContext>
#include <QQmlListReference>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <algorithm>
#include <utility>

namespace qamui {

static QQmlContext *delegateContext(Item *item) {
  return item ? item->findChild<QQmlContext *>(QString(),
                                               Qt::FindDirectChildrenOnly)
              : nullptr;
}

using DelegateUtils::setPropertyIfExists;

GridView::GridView(QObject *parent) : ScrollView(parent) {
  // Scrolling and resizing change the visible window; ask for a layout pass
  // so rebuildViewport() runs before the next paint.
  const auto requestViewport = [this]() {
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
  };
  connect(this, &ScrollView::contentYChanged, this, requestViewport);
  connect(this, &Item::heightChanged, this, requestViewport);
  connect(this, &Item::widthChanged, this, requestViewport);
}

GridView::~GridView() { clearItems(); }

void GridView::setRootIndex(const QModelIndex &rootIndex) {
  if (m_rootIndex == rootIndex)
    return;
  m_rootIndex = rootIndex;
  m_nestedWarned = false;
  emit rootIndexChanged();
  if (qobject_cast<QAbstractItemModel *>(m_objectModel.data()))
    generateItems();
}

void GridView::setModel(const QVariant &model) {
  if (m_model == model)
    return;
  m_nestedWarned = false;
  disconnectModelSignals();
  if (m_objectModel)
    disconnect(m_objectModel, nullptr, this, nullptr);
  ++m_modelGeneration;
  m_model = model;
  m_objectModel =
      model.canConvert<QObject *>() ? model.value<QObject *>() : nullptr;
  if (m_objectModel) {
    connect(m_objectModel, &QObject::destroyed, this, [this]() {
      m_objectModel.clear();
      m_model.clear();
      m_modelCount = 0;
      m_listValues.clear();
      m_roleMap.clear();
      clearItems();
      setContentHeight(0);
      emit modelChanged();
    });
  }
  emit modelChanged();
  generateItems();
}

void GridView::setDelegate(QQmlComponent *delegate) {
  if (m_delegate == delegate)
    return;
  if (m_delegate)
    disconnect(m_delegate, nullptr, this, nullptr);
  m_delegate = delegate;
  if (m_delegate) {
    connect(m_delegate, &QObject::destroyed, this, [this]() {
      m_delegate.clear();
      clearItems();
      setContentHeight(0);
      emit delegateChanged();
    });
    connect(m_delegate, &QQmlComponent::statusChanged, this,
            [this](QQmlComponent::Status status) {
              if (status == QQmlComponent::Ready)
                generateItems();
              else if (status == QQmlComponent::Error)
                DelegateUtils::isReady(m_delegate, this);
            });
  }
  emit delegateChanged();
  generateItems();
}

void GridView::setCellWidth(int width) {
  if (m_cellWidth == width)
    return;
  m_cellWidth = width;
  emit cellWidthChanged();
  updateContentExtent();
  markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::setReuseItems(bool reuse) {
  if (m_reuseItems == reuse)
    return;
  m_reuseItems = reuse;
  emit reuseItemsChanged();
  if (!reuse) {
    for (Item *item : std::as_const(m_reusePool)) {
      item->setParentItem(nullptr);
      delete item;
    }
    m_reusePool.clear();
  }
}

// A delegate that scrolled out: pool it (hidden) or destroy it. Delegates
// picked through `choices` are not pooled because the component may differ
// per row.
void GridView::releaseDelegateItem(Item *item) {
  if (!item)
    return;
  const bool poolable = m_reuseItems && !QQmlListReference(m_delegate, "choices").isValid();
  if (poolable) {
    item->setVisible(false);
    m_reusePool.append(item);
    return;
  }
  item->setParentItem(nullptr);
  delete item;
}

void GridView::setCellHeight(int height) {
  if (m_cellHeight == height)
    return;
  m_cellHeight = height;
  emit cellHeightChanged();
  updateContentExtent();
  markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::clearItems() {
  for (Item *item : std::as_const(m_createdItems)) {
    if (item) {
      item->setParentItem(nullptr);
      delete item;
    }
  }
  m_createdItems.clear();
  for (Item *item : std::as_const(m_reusePool)) {
    item->setParentItem(nullptr);
    delete item;
  }
  m_reusePool.clear();
  m_materializedFirst = -1;
  m_materializedLast = -1;
}

void GridView::disconnectModelSignals() {
  for (const QMetaObject::Connection &connection :
       std::as_const(m_modelConnections))
    disconnect(connection);
  m_modelConnections.clear();
}

int GridView::columnsPerRow() const {
  return m_cellWidth > 0 ? (std::max)(1, width() / m_cellWidth) : 1;
}

void GridView::updateContentExtent() {
  if (m_cellHeight <= 0 || m_modelCount <= 0) {
    setContentHeight(0);
    return;
  }
  const int columns = columnsPerRow();
  const int rows = (m_modelCount + columns - 1) / columns;
  setContentHeight(rows * m_cellHeight);
}

void GridView::reindexItem(Item *item, int index) {
  if (!item)
    return;
  DelegateUtils::setContextProperty(m_delegate, delegateContext(item), "index",
                                    index);
  setPropertyIfExists(item, "index", index);
}

// Publishes row `index` to a delegate: on creation through initial
// properties and a fresh context, on rebind by writing the existing row map,
// context, and required properties in place.
void GridView::bindDelegateItem(Item *item, int index, bool created) {
  Q_UNUSED(created);
  QVariant rowValue;
  QVariantMap rowValues;
  const bool countModel = m_listValues.isEmpty() && !m_objectModel;
  QQmlPropertyMap *rowMap =
      item->findChild<QQmlPropertyMap *>(QString(), Qt::FindDirectChildrenOnly);
  if (!countModel && index < m_listValues.size()) {
    rowValue = m_listValues.at(index);
    QVariant normalized = rowValue;
    if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
      normalized = normalized.value<QJSValue>().toVariant();
    if (normalized.canConvert<QVariantMap>())
      rowValues = normalized.toMap();
  } else if (auto *model =
                 qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
    const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
    for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it)
      rowValues.insert(it.value(), model->data(modelIndex, it.key()));
    NumericItemRoles::apply(model, modelIndex, rowValues, true);
    if (rowMap)
      for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
        rowMap->insert(it.key(), it.value());
  }
  QQmlContext *context = delegateContext(item);
  DelegateUtils::setContextProperty(m_delegate, context, "index", index);
  setPropertyIfExists(item, "index", index);
  if (!rowMap && !countModel) {
    DelegateUtils::setContextProperty(m_delegate, context, "model", rowValue);
    DelegateUtils::setContextProperty(m_delegate, context, "modelData", rowValue);
    setPropertyIfExists(item, "model", rowValue);
    setPropertyIfExists(item, "modelData", rowValue);
  }
  for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
    DelegateUtils::setContextProperty(m_delegate, context, it.key(), it.value());
    setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
  }
}

Item *GridView::createDelegateItem(int index) {
  if (index < 0 || index >= m_modelCount)
    return nullptr;

  QVariant rowValue;
  QVariantMap rowValues;
  QQmlPropertyMap *rowMap = nullptr;
  const bool countModel = m_listValues.isEmpty() && !m_objectModel;
  if (!countModel && index < m_listValues.size()) {
    rowValue = m_listValues.at(index);
    QVariant normalized = rowValue;
    if (normalized.userType() == QMetaType::fromType<QJSValue>().id())
      normalized = normalized.value<QJSValue>().toVariant();
    if (normalized.canConvert<QVariantMap>())
      rowValues = normalized.toMap();
  } else if (auto *model =
                 qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
    rowMap = new QQmlPropertyMap(this);
    const QModelIndex modelIndex = model->index(index, 0, m_rootIndex);
    for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
      const QVariant value = model->data(modelIndex, it.key());
      rowMap->insert(it.value(), value);
      rowValues.insert(it.value(), value);
    }
    NumericItemRoles::apply(model, modelIndex, rowValues);
    for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
      rowMap->insert(it.key(), it.value());
  }

  QQmlComponent *component =
      DelegateUtils::resolveDelegate(m_delegate, index, 0, rowValues);
  if (!DelegateUtils::isReady(component, this)) {
    delete rowMap;
    return nullptr;
  }
  QQmlContext *context = DelegateUtils::createContext(component, this);
  if (!context) {
    delete rowMap;
    return nullptr;
  }

  QVariantMap initialProps;
  initialProps["index"] = index;
  DelegateUtils::setContextProperty(component, context, "index", index);
  if (rowMap) {
    initialProps["model"] = QVariant::fromValue(rowMap);
    initialProps["modelData"] = QVariant::fromValue(rowMap);
    DelegateUtils::setContextProperty(component, context, "model", rowMap);
    DelegateUtils::setContextProperty(component, context, "modelData", rowMap);
  } else if (!countModel) {
    initialProps["model"] = rowValue;
    initialProps["modelData"] = rowValue;
    DelegateUtils::setContextProperty(component, context, "model", rowValue);
    DelegateUtils::setContextProperty(component, context, "modelData",
                                      rowValue);
  }
  for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it) {
    initialProps[it.key()] = it.value();
    DelegateUtils::setContextProperty(component, context, it.key(),
                                      it.value());
  }

  Item *item =
      DelegateUtils::createItem(component, initialProps, context, this, this);
  if (!item) {
    delete rowMap;
    return nullptr;
  }
  if (rowMap)
    rowMap->setParent(item);
  setPropertyIfExists(item, "index", index);
  if (rowMap) {
    setPropertyIfExists(item, "model", QVariant::fromValue(rowMap));
    setPropertyIfExists(item, "modelData", QVariant::fromValue(rowMap));
  } else if (!countModel) {
    setPropertyIfExists(item, "model", rowValue);
    setPropertyIfExists(item, "modelData", rowValue);
  }
  for (auto it = rowValues.cbegin(); it != rowValues.cend(); ++it)
    setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
  return item;
}

void GridView::rebuildViewport(bool force) {
  if (!DelegateUtils::isReady(m_delegate, this)) {
    clearItems();
    return;
  }
  if (m_modelCount <= 0) {
    clearItems();
    maybeFetchMore();
    return;
  }

  // Without cell geometry there is no viewport to compute; fall back to
  // instantiating every row, as before virtualization.
  int first = 0;
  int last = m_modelCount - 1;
  if (m_cellWidth > 0 && m_cellHeight > 0) {
    const int columns = columnsPerRow();
    const int viewportHeight =
        (std::max)(1, height() - topPadding() - bottomPadding());
    const int firstRow = (std::max)(0, contentY() / m_cellHeight);
    const int lastRow = (contentY() + viewportHeight - 1) / m_cellHeight;
    constexpr int overscanRows = 1;
    first = (std::max)(0, (firstRow - overscanRows) * columns);
    last = (std::min)(m_modelCount - 1, (lastRow + overscanRows + 1) * columns - 1);
  }

  if (!force && first == m_materializedFirst && last == m_materializedLast)
    return;

  for (auto it = m_createdItems.begin(); it != m_createdItems.end();) {
    if (it.key() >= first && it.key() <= last) {
      ++it;
      continue;
    }
    Item *item = it.value();
    it = m_createdItems.erase(it);
    releaseDelegateItem(item);
  }
  for (int index = first; index <= last; ++index) {
    if (m_createdItems.contains(index))
      continue;
    if (!m_reusePool.isEmpty()) {
      Item *item = m_reusePool.takeLast();
      bindDelegateItem(item, index, false);
      item->setVisible(true);
      m_createdItems.insert(index, item);
    } else if (Item *item = createDelegateItem(index)) {
      m_createdItems.insert(index, item);
    }
  }
  m_materializedFirst = first;
  m_materializedLast = last;
  maybeFetchMore();
  markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void GridView::maybeFetchMore() {
  auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data());
  if (!model)
    return;
  const int threshold = columnsPerRow(); // one overscan row
  DelegateUtils::fetchMoreIfNeeded(model, m_rootIndex, threshold, m_fetching,
                                   [this] { return m_materializedLast; });
}

void GridView::generateItems() {
  clearItems();
  disconnectModelSignals();
  m_listValues.clear();
  m_roleMap.clear();
  m_modelCount = 0;
  if (!DelegateUtils::isReady(m_delegate, this)) {
    setContentHeight(0);
    return;
  }

  if (DelegateUtils::unpackStaticModel(m_model, m_modelCount, m_listValues)) {
    // Model count and list values unpacked
  } else if (auto *model =
                 qobject_cast<QAbstractItemModel *>(m_objectModel.data())) {
    DelegateUtils::warnIfNested(model, m_rootIndex, this, m_nestedWarned);
    const QHash<int, QByteArray> roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
      m_roleMap.insert(it.key(), QString::fromUtf8(it.value()));
    m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));

    const quint64 generation = m_modelGeneration;
    QPointer<QAbstractItemModel> guardedModel(model);
    const auto live = [this, guardedModel, generation] {
      return guardedModel && generation == m_modelGeneration &&
             guardedModel.data() == m_objectModel.data();
    };
    m_modelConnections.append(connect(
        model, &QAbstractItemModel::dataChanged, this,
        [this, guardedModel, live](const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight,
                                   const QList<int> &changedRoles) {
          if (!live() || topLeft.parent() != m_rootIndex)
            return;
          for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
            Item *item = m_createdItems.value(row, nullptr);
            if (!item)
              continue;
            // Bound delegates have no per-row context; the row map is
            // parented to the item in both cases.
            QQmlContext *context = delegateContext(item);
            auto *rowMap = item->findChild<QQmlPropertyMap *>(
                QString(), Qt::FindDirectChildrenOnly);
            const QModelIndex index = guardedModel->index(row, 0, m_rootIndex);
            for (auto it = m_roleMap.cbegin(); it != m_roleMap.cend(); ++it) {
              if (!changedRoles.isEmpty() && !changedRoles.contains(it.key()))
                continue;
              const QVariant value = guardedModel->data(index, it.key());
              if (rowMap)
                rowMap->insert(it.value(), value);
              DelegateUtils::setContextProperty(m_delegate, context,
                                                it.value(), value);
              setPropertyIfExists(item, it.value().toUtf8().constData(),
                                  value);
            }
            QVariantMap standard;
            NumericItemRoles::apply(guardedModel, index, standard, true);
            for (auto it = standard.cbegin(); it != standard.cend(); ++it) {
              if (rowMap)
                rowMap->insert(it.key(), it.value());
              DelegateUtils::setContextProperty(m_delegate, context, it.key(),
                                                it.value());
              setPropertyIfExists(item, it.key().toUtf8().constData(),
                                  it.value());
            }
          }
          markDirty();
        }));
    m_modelConnections.append(connect(
        model, &QAbstractItemModel::rowsInserted, this,
        [this, guardedModel, live](const QModelIndex &parent, int first,
                                   int last) {
          if (!live() || parent != m_rootIndex)
            return;
          insertModelRows(guardedModel, first, last);
        }));
    m_modelConnections.append(connect(
        model, &QAbstractItemModel::rowsRemoved, this,
        [this, guardedModel, live](const QModelIndex &parent, int first,
                                   int last) {
          if (!live() || parent != m_rootIndex)
            return;
          removeModelRows(first, last);
        }));
    m_modelConnections.append(connect(
        model, &QAbstractItemModel::rowsMoved, this,
        [this, live](const QModelIndex &sourceParent, int, int,
                     const QModelIndex &destinationParent, int) {
          if (!live())
            return;
          // Any move touching the shown level changes the row order or set;
          // rebuilding the window is cheap because it is bounded by the viewport.
          if (sourceParent == m_rootIndex || destinationParent == m_rootIndex)
            refreshFromModel();
        }));
    m_modelConnections.append(connect(model, &QAbstractItemModel::layoutChanged,
                                      this, [this, live]() {
                                        if (live())
                                          refreshFromModel();
                                      }));
    m_modelConnections.append(connect(model, &QAbstractItemModel::modelReset,
                                      this, [this, live]() {
                                        if (live())
                                          generateItems();
                                      }));
  }

  updateContentExtent();
  rebuildViewport(true);
  markDirty();
}

void GridView::refreshFromModel() {
  if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data()))
    m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
  clearItems();
  updateContentExtent();
  rebuildViewport(true);
}

void GridView::insertModelRows(QAbstractItemModel *model, int first, int last) {
  if (!model || !m_delegate || first < 0 || first > last)
    return;
  m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
  const int count = last - first + 1;
  QMap<int, Item *> shifted;
  for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
    const int index = it.key() >= first ? it.key() + count : it.key();
    if (index != it.key())
      reindexItem(it.value(), index);
    shifted.insert(index, it.value());
  }
  m_createdItems = std::move(shifted);
  updateContentExtent();
  rebuildViewport(true);
}

void GridView::removeModelRows(int first, int last) {
  if (first < 0 || first > last) return;
  if (auto *model = qobject_cast<QAbstractItemModel *>(m_objectModel.data()))
    m_modelCount = (std::max)(0, model->rowCount(m_rootIndex));
  else
    m_modelCount = (std::max)(0, m_modelCount - (last - first + 1));
  const int count = last - first + 1;
  QMap<int, Item *> shifted;
  for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
    Item *item = it.value();
    if (it.key() >= first && it.key() <= last) {
      releaseDelegateItem(item);
      continue;
    }
    const int index = it.key() > last ? it.key() - count : it.key();
    if (index != it.key())
      reindexItem(item, index);
    shifted.insert(index, item);
  }
  m_createdItems = std::move(shifted);
  updateContentExtent();
  rebuildViewport(true);
}

void GridView::layoutChildren() {
  if (m_cellWidth <= 0 || m_cellHeight <= 0)
    return;
  const int columns = columnsPerRow();
  for (auto it = m_createdItems.cbegin(); it != m_createdItems.cend(); ++it) {
    Item *childItem = it.value();
    if (!childItem)
      continue;
    // Cells are addressed by model index so a hidden delegate keeps its slot,
    // as in Qt Quick's GridView.
    childItem->setX((it.key() % columns) * m_cellWidth);
    childItem->setY((it.key() / columns) * m_cellHeight);
  }
  updateContentExtent();
}

int GridView::implicitWidth() const {
  if (m_cellWidth > 0 && m_modelCount > 0)
    return m_cellWidth + leftPadding() + rightPadding(); // At least 1 column
  return leftPadding() + rightPadding();
}

int GridView::implicitHeight() const {
  if (m_cellHeight > 0 && m_modelCount > 0)
    return m_cellHeight + topPadding() + bottomPadding(); // At least 1 row
  return topPadding() + bottomPadding();
}

void GridView::updateLayout() {
  ScrollView::updateLayout();
  rebuildViewport();
  layoutChildren();
}

void GridView::paint(TextGridBuffer &buffer, int absX, int absY,
                      const ClipRect &clipRect) const {
  if (!visible())
    return;
  ScrollView::paint(buffer, absX, absY, clipRect);
}

} // namespace qamui
