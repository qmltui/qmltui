#include <QmlTui/Core/Items/Scrollback.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/DelegateUtils.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Engine/ItemPaintTraversal_p.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QQmlContext>
#include <QQmlListReference>
#include <algorithm>
#include <memory>

namespace qmltui {

static bool isDelegateReady(QQmlComponent* delegate, QObject* owner) {
    if (!delegate) return false;
    if (QQmlListReference(delegate, "choices").isValid()) return true;
    return DelegateUtils::isReady(delegate, owner);
}

static int resolveItemSpacing(Item* item, int defaultSpacing) {
    if (!item) return defaultSpacing;
    const QVariant prop = item->property("streamSpacing");
    if (prop.isValid() && prop.canConvert<int>()) {
        const int s = prop.toInt();
        if (s >= 0) return s;
    }
    return defaultSpacing;
}

static QList<Span> normalizeSpansForScrollback(const QList<Span>& spans) {
    QList<Span> normalized = spans;
    for (Span& span : normalized) {
        if (span.text().contains(u'\r')) {
            QString text = span.text();
            text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
            text.remove(u'\r');
            span.setText(text);
        }
    }
    for (qsizetype i = normalized.size() - 1; i >= 0; --i) {
        const QString text = normalized[i].text();
        if (!text.isEmpty()) {
            if (text.endsWith(u'\n')) {
                normalized[i].setText(text.chopped(1));
            }
            break;
        }
    }
    if (normalized.isEmpty()) {
        normalized.append(Span());
    }
    return normalized;
}

Scrollback::Scrollback(QObject* parent) : QObject(parent) {}

Scrollback::~Scrollback() {
    clearDelegateCache();
}

void Scrollback::componentComplete() {
    if (auto* parentItem = qobject_cast<Item*>(parent())) {
        connect(parentItem, &Item::ancestryChanged, this, &Scrollback::appendExistingRows,
                Qt::UniqueConnection);
    }
    appendExistingRows();
}

void Scrollback::setModel(QAbstractItemModel* model) {
    setModel(QVariant::fromValue(static_cast<QObject*>(model)));
}

void Scrollback::setModel(const QVariant& model) {
    if (m_modelVariant == model) return;

    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }

    m_modelVariant = model;
    m_model = qvariant_cast<QAbstractItemModel*>(model);
    if (!m_model && model.canConvert<QObject*>()) {
        m_model = qobject_cast<QAbstractItemModel*>(model.value<QObject*>());
    }
    m_consumedRowCount = 0;

    if (m_model) {
        connect(m_model, &QObject::destroyed, this, [this]() {
            m_model.clear();
            m_modelVariant.clear();
            m_consumedRowCount = 0;
            emit modelChanged();
        });
        connect(m_model, &QAbstractItemModel::rowsInserted, this,
                [this](const QModelIndex& parent, int first, int last) {
                    if (parent.isValid()) {
                        qmlWarning(this)
                            << "Scrollback ignores rows inserted below the model root.";
                        return;
                    }
                    Q_UNUSED(first);
                    Q_UNUSED(last);
                    appendExistingRows();
                });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this,
                [this]() { m_consumedRowCount = m_model ? m_model->rowCount() : 0; });
        connect(m_model, &QAbstractItemModel::modelReset, this, [this]() {
            m_consumedRowCount = 0;
            appendExistingRows();
        });
    }

    emit modelChanged();
    appendExistingRows();
}

void Scrollback::setDelegate(QQmlComponent* delegate) {
    if (m_delegate == delegate) return;
    if (m_delegate) disconnect(m_delegate, nullptr, this, nullptr);
    clearDelegateCache();
    m_delegate = delegate;
    if (m_delegate) {
        connect(m_delegate, &QObject::destroyed, this, [this]() {
            clearDelegateCache();
            m_delegate.clear();
            emit delegateChanged();
        });
        connect(m_delegate, &QQmlComponent::statusChanged, this,
                [this](QQmlComponent::Status status) {
                    if (status == QQmlComponent::Ready)
                        appendExistingRows();
                    else if (status == QQmlComponent::Error)
                        DelegateUtils::isReady(m_delegate, this);
                });
    }
    emit delegateChanged();
    appendExistingRows();
}

void Scrollback::setSpacing(int spacing) {
    spacing = (std::max)(0, spacing);
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
}

void Scrollback::setSeparator(QQmlComponent* separator) {
    if (m_separator == separator) return;
    if (m_separator) disconnect(m_separator, nullptr, this, nullptr);
    if (m_cachedSeparator.item) {
        delete m_cachedSeparator.item;
        m_cachedSeparator = {nullptr, nullptr};
    }
    m_separator = separator;
    if (m_separator) {
        connect(m_separator, &QObject::destroyed, this, [this]() {
            if (m_cachedSeparator.item) {
                delete m_cachedSeparator.item;
                m_cachedSeparator = {nullptr, nullptr};
            }
            m_separator.clear();
            emit separatorChanged();
        });
    }
    emit separatorChanged();
}

void Scrollback::setReuseItems(bool reuse) {
    if (m_reuseItems == reuse) return;
    m_reuseItems = reuse;
    if (!m_reuseItems) {
        clearDelegateCache();
    }
    emit reuseItemsChanged();
}

void Scrollback::clear() {
    m_consumedRowCount = 0;
    m_hasRenderedFirstItem = false;
    clearDelegateCache();
}

void Scrollback::clearDelegateCache() {
    for (auto it = m_delegateCache.begin(); it != m_delegateCache.end(); ++it) {
        if (it.value().item) {
            delete it.value().item;
        }
    }
    m_delegateCache.clear();
    if (m_cachedSeparator.item) {
        delete m_cachedSeparator.item;
        m_cachedSeparator = {nullptr, nullptr};
    }
}

Scrollback::CachedDelegate Scrollback::getOrCreateDelegate(QQmlComponent* component,
                                                           const QVariantMap& initialProperties) {
    if (!component) return {nullptr, nullptr};
    auto it = m_delegateCache.find(component);
    if (it != m_delegateCache.end() && it.value().item) {
        return it.value();
    }

    QQmlContext* context = DelegateUtils::createContext(component, this);
    if (!context) return {nullptr, nullptr};

    for (auto itProp = initialProperties.cbegin(); itProp != initialProperties.cend(); ++itProp) {
        DelegateUtils::setContextProperty(component, context, itProp.key(), itProp.value());
    }

    Item* item = DelegateUtils::createItem(component, initialProperties, context, terminal(), this);
    if (!item) return {nullptr, nullptr};
    item->setParentItem(nullptr);
    item->setParent(this);

    CachedDelegate cached{item, context};
    m_delegateCache.insert(component, cached);
    return cached;
}

Scrollback::CachedDelegate Scrollback::getOrCreateSeparator() {
    if (!m_separator || !DelegateUtils::isReady(m_separator, this)) return {nullptr, nullptr};

    if (m_cachedSeparator.item) return m_cachedSeparator;

    QQmlContext* context = DelegateUtils::createContext(m_separator, this);
    if (!context) return {nullptr, nullptr};

    Item* item = DelegateUtils::createItem(m_separator, {}, context, terminal(), this);
    if (!item) return {nullptr, nullptr};
    item->setParentItem(nullptr);
    item->setParent(this);

    m_cachedSeparator = CachedDelegate{item, context};
    return m_cachedSeparator;
}

int Scrollback::resolvedWidth(Item* item, Terminal* output) {
    int w = item->width() > 0 ? item->width() : (output ? output->width() : 80);
    if (w <= 0) w = 80;
    if (item->width() <= 0) {
        item->setWidth(w);
    }
    return w;
}

int Scrollback::resolvedHeight(Item* item) {
    const int h = item->height() > 0 ? item->height() : item->implicitHeight();
    return h <= 0 ? 1 : h;
}

Item* Scrollback::separatorItem(Terminal* output, std::unique_ptr<Item>& owned) {
    if (!m_separator || !DelegateUtils::isReady(m_separator, this)) return nullptr;

    if (m_reuseItems) {
        CachedDelegate cd = getOrCreateSeparator();
        return cd.item;
    }

    QQmlContext* context = DelegateUtils::createContext(m_separator, this);
    if (!context) return nullptr;
    Item* item = DelegateUtils::createItem(m_separator, {}, context, output, this);
    if (!item) return nullptr;
    item->setParentItem(nullptr);
    owned.reset(item);
    return item;
}

bool Scrollback::submitBatch(Item* item, Terminal* output) {
    if (!item || !output) return false;
    std::unique_ptr<Item> ownedSeparator;
    Item* separator = m_hasRenderedFirstItem ? separatorItem(output, ownedSeparator) : nullptr;
    const int spacing = m_hasRenderedFirstItem ? resolveItemSpacing(item, m_spacing) : 0;
    const int itemWidth = resolvedWidth(item, output);
    const int itemHeight = resolvedHeight(item);
    const int separatorWidth = separator ? resolvedWidth(separator, output) : 0;
    const int separatorHeight = separator ? resolvedHeight(separator) : 0;

    TextGridBuffer buffer;
    if (!buffer.resize((std::max)(itemWidth, separatorWidth),
                       separatorHeight + spacing + itemHeight))
        return reportRejection(QStringLiteral("Scrollback item is too large to paint"));
    if (separator) {
        ItemPaintTraversal::paintSubtree(separator, buffer, 0, 0,
                                         ClipRect{0, 0, separatorWidth, separatorHeight});
    }
    const int itemRow = separatorHeight + spacing;
    ItemPaintTraversal::paintSubtree(item, buffer, 0, itemRow,
                                     ClipRect{0, itemRow, itemWidth, itemHeight});

    TerminalEngine* engine = output->engine();
    if (!engine)
        return reportRejection(QStringLiteral("Scrollback requires a root Terminal session"));
    if (!engine->submitScrollback(buffer))
        return reportRejection(
            QStringLiteral("Scrollback queue limit (1 MiB) exceeded; submission rejected"));
    m_hasRenderedFirstItem = true;
    return true;
}

void Scrollback::applyModelDataToDelegate(Item* item, QQmlContext* context,
                                          QQmlComponent* component, int index,
                                          const QVariantMap& modelMap, const QVariant& rawData) {
    if (!item) return;

    DelegateUtils::setPropertyIfExists(item, "index", index);
    DelegateUtils::setPropertyIfExists(item, "model", modelMap);
    DelegateUtils::setPropertyIfExists(item, "modelData",
                                       rawData.isValid() ? rawData : QVariant::fromValue(modelMap));

    for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it) {
        DelegateUtils::setPropertyIfExists(item, it.key().toUtf8().constData(), it.value());
    }

    if (context && component) {
        DelegateUtils::setContextProperty(component, context, QStringLiteral("index"), index);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("model"), modelMap);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("modelData"),
                                          rawData.isValid() ? rawData
                                                            : QVariant::fromValue(modelMap));
        for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it) {
            DelegateUtils::setContextProperty(component, context, it.key(), it.value());
        }
    }
}

void Scrollback::appendExistingRows() {
    if (!m_model || !isDelegateReady(m_delegate, this) || !terminal()) return;

    const int lastRow = m_model->rowCount() - 1;
    if (m_consumedRowCount <= lastRow) {
        m_consumedRowCount = appendRows(m_consumedRowCount, lastRow);
    }
    // Once caught up, ask a paged model for its next page; rowsInserted brings
    // the new rows back through this function.
    DelegateUtils::fetchMoreIfNeeded(m_model.data(), QModelIndex(), 0, m_fetching,
                                     [this] { return m_consumedRowCount - 1; });
}

int Scrollback::appendRows(int first, int last) {
    Terminal* output = terminal();
    if (!m_model || !m_delegate || !output) return first;

    const QHash<int, QByteArray> roles = m_model->roleNames();
    QList<std::pair<int, QString>> roleList;
    roleList.reserve(roles.size());
    for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
        roleList.append({role.key(), QString::fromUtf8(role.value())});
    }

    for (int row = first; row <= last; ++row) {
        const QModelIndex index = m_model->index(row, 0);
        QQmlComponent* rowDelegate = delegateForRow(row);
        if (!DelegateUtils::isReady(rowDelegate, this)) return row;

        QVariantMap properties;
        QVariantMap modelMap;
        properties.insert(QStringLiteral("index"), row);
        for (const auto& [roleKey, roleName] : roleList) {
            const QVariant value = m_model->data(index, roleKey);
            properties.insert(roleName, value);
            modelMap.insert(roleName, value);
        }
        properties.insert(QStringLiteral("model"), modelMap);
        properties.insert(QStringLiteral("modelData"), modelMap);

        Item* itemToRender = nullptr;
        std::unique_ptr<Item> itemCleanup;

        if (m_reuseItems) {
            CachedDelegate cd = getOrCreateDelegate(rowDelegate, properties);
            if (!cd.item) return row;
            applyModelDataToDelegate(cd.item, cd.context, rowDelegate, row, modelMap, modelMap);
            itemToRender = cd.item;
        } else {
            QQmlContext* context = DelegateUtils::createContext(rowDelegate, this);
            if (!context) return row;
            DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("index"), row);
            DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("model"),
                                              modelMap);
            DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("modelData"),
                                              modelMap);
            for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it)
                DelegateUtils::setContextProperty(rowDelegate, context, it.key(), it.value());

            Item* item = DelegateUtils::createItem(rowDelegate, properties, context, output, this);
            if (!item) return row;
            item->setParentItem(nullptr);
            itemCleanup.reset(item);
            itemToRender = item;
        }

        if (!submitBatch(itemToRender, output)) return row;
    }
    return last + 1;
}

void Scrollback::append(const QVariant& data) {
    if (auto* item = qvariant_cast<Item*>(data)) {
        append(item);
        return;
    }
    if (data.canConvert<QObject*>()) {
        if (auto* item = qobject_cast<Item*>(data.value<QObject*>())) {
            append(item);
            return;
        }
    }

    QList<Span> spans;
    if (Span::tryExtractList(data, !m_delegate, spans)) {
        append(spans);
        return;
    }

    Terminal* output = terminal();
    if (!m_delegate || !output) return;

    QQmlComponent* rowDelegate = m_delegate;
    QQmlListReference choices(m_delegate, "choices");
    QVariantMap modelMap;
    QVariant rawData = data;

    if (data.canConvert<QVariantMap>()) {
        modelMap = data.toMap();
    } else if (data.canConvert<QString>()) {
        modelMap.insert(QStringLiteral("display"), data);
    } else {
        modelMap.insert(QStringLiteral("modelData"), data);
    }

    if (choices.isValid()) {
        const QString roleName = m_delegate->property("role").toString();
        const QVariant roleValue = modelMap.value(roleName);
        for (qsizetype choiceIndex = 0; choiceIndex < choices.count(); ++choiceIndex) {
            QObject* choice = choices.at(choiceIndex);
            if (!choice) continue;
            const QVariant choiceValue = choice->property("roleValue");
            if (!choiceValue.isValid() || choiceValue == roleValue) {
                const QVariant prop = choice->property("delegate");
                if (auto* comp = prop.value<QQmlComponent*>()) {
                    rowDelegate = comp;
                    break;
                }
                if (auto* obj = prop.value<QObject*>()) {
                    if (auto* comp = qobject_cast<QQmlComponent*>(obj)) {
                        rowDelegate = comp;
                        break;
                    }
                }
            }
        }
    }

    if (!DelegateUtils::isReady(rowDelegate, this)) return;

    const int currentIndex = m_consumedRowCount;
    QVariantMap properties = modelMap;
    properties.insert(QStringLiteral("index"), currentIndex);
    properties.insert(QStringLiteral("model"), modelMap);
    properties.insert(QStringLiteral("modelData"), rawData);

    Item* itemToRender = nullptr;
    std::unique_ptr<Item> itemCleanup;

    if (m_reuseItems) {
        CachedDelegate cd = getOrCreateDelegate(rowDelegate, properties);
        if (!cd.item) return;
        applyModelDataToDelegate(cd.item, cd.context, rowDelegate, currentIndex, modelMap, rawData);
        itemToRender = cd.item;
    } else {
        QQmlContext* context = DelegateUtils::createContext(rowDelegate, this);
        if (!context) return;
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("index"),
                                          currentIndex);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("model"), modelMap);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("modelData"),
                                          rawData);
        for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it)
            DelegateUtils::setContextProperty(rowDelegate, context, it.key(), it.value());

        Item* item = DelegateUtils::createItem(rowDelegate, properties, context, output, this);
        if (!item) return;
        item->setParentItem(nullptr);
        itemCleanup.reset(item);
        itemToRender = item;
    }

    if (!submitBatch(itemToRender, output)) return;
    ++m_consumedRowCount;
}

void Scrollback::append(qmltui::Item* item) {
    Terminal* output = terminal();
    if (!item || !output) return;

    if (!submitBatch(item, output)) return;
    ++m_consumedRowCount;
}

void Scrollback::append(const qmltui::Span& span) {
    append(QList<Span>{span});
}

void Scrollback::append(const QList<qmltui::Span>& spans) {
    Terminal* output = terminal();
    if (!output) {
        reportRejection(QStringLiteral("Scrollback must belong to a Terminal"));
        return;
    }

    Text textItem;
    textItem.setSpans(normalizeSpansForScrollback(spans));
    textItem.setWidth((std::max)(1, textItem.implicitWidth()));
    textItem.setHeight((std::max)(1, textItem.implicitHeight()));

    if (!submitBatch(&textItem, output)) return;
    ++m_consumedRowCount;
}

bool Scrollback::print(const QVariant& value) {
    if (value.metaType() == QMetaType::fromType<QString>()) {
        return print(value.toString());
    }
    QList<Span> spans;
    if (Span::tryExtractList(value, /*allowVariantMap=*/true, spans)) {
        return print(spans);
    }
    if (value.canConvert<QString>()) {
        return print(value.toString());
    }
    return reportRejection(QStringLiteral("Scrollback.print: unsupported value type"));
}

bool Scrollback::print(const QString& text) {
    Terminal* output = terminal();
    if (!output) return reportRejection(QStringLiteral("Scrollback must belong to a Terminal"));
    if (!output->engine())
        return reportRejection(QStringLiteral("Scrollback requires a root Terminal session"));
    if (!output->engine()->submitScrollback(text))
        return reportRejection(
            QStringLiteral("Scrollback queue limit (1 MiB) exceeded; submission rejected"));
    return true;
}

bool Scrollback::print(const qmltui::Span& span) {
    return print(QList<Span>{span});
}

bool Scrollback::print(const QList<qmltui::Span>& spans) {
    Terminal* output = terminal();
    if (!output) return reportRejection(QStringLiteral("Scrollback must belong to a Terminal"));
    TerminalEngine* engine = output->engine();
    if (!engine)
        return reportRejection(QStringLiteral("Scrollback requires a root Terminal session"));

    Text textItem;
    textItem.setSpans(normalizeSpansForScrollback(spans));
    const int itemWidth = (std::max)(1, textItem.implicitWidth());
    const int itemHeight = (std::max)(1, textItem.implicitHeight());
    textItem.setWidth(itemWidth);
    textItem.setHeight(itemHeight);

    TextGridBuffer buffer;
    if (!buffer.resize(itemWidth, itemHeight))
        return reportRejection(QStringLiteral("Scrollback item is too large to paint"));
    ItemPaintTraversal::paintSubtree(&textItem, buffer, 0, 0,
                                     ClipRect{0, 0, itemWidth, itemHeight});
    if (!engine->submitScrollback(buffer))
        return reportRejection(
            QStringLiteral("Scrollback queue limit (1 MiB) exceeded; submission rejected"));
    return true;
}

bool Scrollback::reportRejection(const QString& reason) {
    qmlWarning(this) << reason;
    emit outputRejected(reason);
    return false;
}

QQmlComponent* Scrollback::delegateForRow(int row) const {
    QQmlListReference choices(m_delegate, "choices");
    if (!choices.isValid()) {
        return m_delegate;
    }

    const QModelIndex modelIndex = m_model->index(row, 0);
    const QString roleName = m_delegate->property("role").toString();
    QVariant roleValue;
    const QByteArray roleUtf8 = roleName.toUtf8();
    const QHash<int, QByteArray> roles = m_model->roleNames();
    for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
        if (role.value() == roleUtf8) {
            roleValue = m_model->data(modelIndex, role.key());
            break;
        }
    }

    for (qsizetype choiceIndex = 0; choiceIndex < choices.count(); ++choiceIndex) {
        QObject* choice = choices.at(choiceIndex);
        if (!choice) continue;

        const QVariant rProp = choice->property("row");
        const QVariant cProp = choice->property("column");
        const int choiceRow = rProp.isValid() && !rProp.isNull() ? rProp.toInt() : -1;
        const int choiceColumn = cProp.isValid() && !cProp.isNull() ? cProp.toInt() : -1;
        const QVariant choiceValue = choice->property("roleValue");
        const bool rowMatches = choiceRow < 0 || choiceRow == row;
        const bool columnMatches = choiceColumn < 0 || choiceColumn == 0;
        const bool roleMatches = !choiceValue.isValid() || choiceValue == roleValue;
        if (!rowMatches || !columnMatches || !roleMatches) continue;

        const QVariant prop = choice->property("delegate");
        if (auto* comp = prop.value<QQmlComponent*>()) return comp;
        if (auto* obj = prop.value<QObject*>()) return qobject_cast<QQmlComponent*>(obj);
    }

    return nullptr;
}

Terminal* Scrollback::terminal() const {
    QObject* current = parent();
    while (current) {
        if (auto* result = qobject_cast<Terminal*>(current)) {
            return result;
        }
        current = current->parent();
    }
    return nullptr;
}

} // namespace qmltui
