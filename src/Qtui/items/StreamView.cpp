#include "StreamView.h"
#include "DelegateUtils.h"
#include "Item.h"
#include "../engine/ItemPaintTraversal_p.h"
#include "Terminal.h"
#include "../primitives/TextGridBuffer.h"
#include <QQmlContext>
#include <QQmlListReference>
#include <memory>

namespace qtui {

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

StreamView::StreamView(QObject* parent)
    : QObject(parent) {
}

StreamView::~StreamView() {
    clearDelegateCache();
}

void StreamView::setModel(QAbstractItemModel* model) {
    setModel(QVariant::fromValue(static_cast<QObject*>(model)));
}

void StreamView::setModel(const QVariant& model) {
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
                        qWarning("StreamView ignores rows inserted below the model root.");
                        return;
                    }
                    Q_UNUSED(first);
                    Q_UNUSED(last);
                    appendExistingRows();
                });
        connect(m_model, &QAbstractItemModel::rowsRemoved, this,
                [this]() {
                    m_consumedRowCount = m_model ? m_model->rowCount() : 0;
                });
        connect(m_model, &QAbstractItemModel::modelReset, this,
                [this]() {
                    m_consumedRowCount = 0;
                    appendExistingRows();
                });
    }

    emit modelChanged();
    appendExistingRows();
}

void StreamView::setDelegate(QQmlComponent* delegate) {
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

void StreamView::setSpacing(int spacing) {
    spacing = std::max(0, spacing);
    if (m_spacing == spacing) return;
    m_spacing = spacing;
    emit spacingChanged();
}

void StreamView::setSeparator(QQmlComponent* separator) {
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

void StreamView::setReuseItems(bool reuse) {
    if (m_reuseItems == reuse) return;
    m_reuseItems = reuse;
    if (!m_reuseItems) {
        clearDelegateCache();
    }
    emit reuseItemsChanged();
}

void StreamView::clear() {
    m_consumedRowCount = 0;
    m_hasRenderedFirstItem = false;
    clearDelegateCache();
}

void StreamView::clearDelegateCache() {
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

StreamView::CachedDelegate StreamView::getOrCreateDelegate(QQmlComponent* component, const QVariantMap& initialProperties) {
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

StreamView::CachedDelegate StreamView::getOrCreateSeparator() {
    if (!m_separator || !DelegateUtils::isReady(m_separator, this))
        return {nullptr, nullptr};

    if (m_cachedSeparator.item)
        return m_cachedSeparator;

    QQmlContext* context = DelegateUtils::createContext(m_separator, this);
    if (!context) return {nullptr, nullptr};

    Item* item = DelegateUtils::createItem(m_separator, {}, context, terminal(), this);
    if (!item) return {nullptr, nullptr};
    item->setParentItem(nullptr);
    item->setParent(this);

    m_cachedSeparator = CachedDelegate{item, context};
    return m_cachedSeparator;
}

std::string StreamView::renderItemToAnsi(Item* item, Terminal* output) {
    if (!item) return "";
    int w = item->width() > 0 ? item->width() : (output ? output->width() : 80);
    if (w <= 0) w = 80;
    if (item->width() <= 0) {
        item->setWidth(w);
    }
    int h = item->height() > 0 ? item->height() : item->implicitHeight();
    if (h <= 0) h = 1;

    TextGridBuffer buffer;
    buffer.resize(w, h);
    ItemPaintTraversal::paintSubtree(item, buffer, 0, 0);
    return buffer.toAnsiString();
}

std::string StreamView::renderSeparatorToAnsi(Terminal* output) {
    if (!m_separator || !DelegateUtils::isReady(m_separator, this)) return "";

    if (m_reuseItems) {
        CachedDelegate cd = getOrCreateSeparator();
        if (!cd.item) return "";
        return renderItemToAnsi(cd.item, output);
    }

    QQmlContext* context = DelegateUtils::createContext(m_separator, this);
    if (!context) return "";
    Item* item = DelegateUtils::createItem(m_separator, {}, context, output, this);
    if (!item) return "";
    item->setParentItem(nullptr);
    std::string ansi = renderItemToAnsi(item, output);
    delete item;
    return ansi;
}

void StreamView::applyModelDataToDelegate(Item* item, QQmlContext* context, QQmlComponent* component,
                                         int index, const QVariantMap& modelMap, const QVariant& rawData) {
    if (!item) return;

    item->setProperty("index", index);
    item->setProperty("model", modelMap);
    item->setProperty("modelData", rawData.isValid() ? rawData : QVariant::fromValue(modelMap));

    for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it) {
        item->setProperty(it.key().toUtf8().constData(), it.value());
    }

    if (context && component) {
        DelegateUtils::setContextProperty(component, context, QStringLiteral("index"), index);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("model"), modelMap);
        DelegateUtils::setContextProperty(component, context, QStringLiteral("modelData"),
                                          rawData.isValid() ? rawData : QVariant::fromValue(modelMap));
        for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it) {
            DelegateUtils::setContextProperty(component, context, it.key(), it.value());
        }
    }
}

void StreamView::appendExistingRows() {
    if (!m_model || !isDelegateReady(m_delegate, this) || !terminal()) return;

    const int lastRow = m_model->rowCount() - 1;
    if (m_consumedRowCount <= lastRow) {
        m_consumedRowCount = appendRows(m_consumedRowCount, lastRow);
    }
}

int StreamView::appendRows(int first, int last) {
    Terminal* output = terminal();
    if (!m_model || !m_delegate || !output) return first;

    const QHash<int, QByteArray> roles = m_model->roleNames();
    std::string batchAnsi;

    for (int row = first; row <= last; ++row) {
        const QModelIndex index = m_model->index(row, 0);
        QQmlComponent* rowDelegate = delegateForRow(row);
        if (!DelegateUtils::isReady(rowDelegate, this)) return row;

        QVariantMap properties;
        QVariantMap modelMap;
        properties.insert(QStringLiteral("index"), row);
        for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
            const QString roleName = QString::fromUtf8(role.value());
            const QVariant value = m_model->data(index, role.key());
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
            DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("model"), modelMap);
            DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("modelData"), modelMap);
            for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it)
                DelegateUtils::setContextProperty(rowDelegate, context, it.key(), it.value());

            Item* item = DelegateUtils::createItem(rowDelegate, properties, context, output, this);
            if (!item) return row;
            item->setParentItem(nullptr);
            itemCleanup.reset(item);
            itemToRender = item;
        }

        if (m_hasRenderedFirstItem) {
            if (m_separator) {
                std::string sepAnsi = renderSeparatorToAnsi(output);
                if (!sepAnsi.empty()) batchAnsi += sepAnsi;
            }
            const int effectiveSpacing = resolveItemSpacing(itemToRender, m_spacing);
            for (int s = 0; s < effectiveSpacing; ++s) batchAnsi += "\r\n";
        }

        batchAnsi += renderItemToAnsi(itemToRender, output);
        m_hasRenderedFirstItem = true;
    }

    if (!batchAnsi.empty()) {
        output->print(QString::fromStdString(batchAnsi));
    }
    return last + 1;
}

void StreamView::append(const QVariant& data) {
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
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("index"), currentIndex);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("model"), modelMap);
        DelegateUtils::setContextProperty(rowDelegate, context, QStringLiteral("modelData"), rawData);
        for (auto it = modelMap.cbegin(); it != modelMap.cend(); ++it)
            DelegateUtils::setContextProperty(rowDelegate, context, it.key(), it.value());

        Item* item = DelegateUtils::createItem(rowDelegate, properties, context, output, this);
        if (!item) return;
        item->setParentItem(nullptr);
        itemCleanup.reset(item);
        itemToRender = item;
    }

    std::string batchAnsi;
    if (m_hasRenderedFirstItem) {
        if (m_separator) {
            std::string sepAnsi = renderSeparatorToAnsi(output);
            if (!sepAnsi.empty()) batchAnsi += sepAnsi;
        }
        const int effectiveSpacing = resolveItemSpacing(itemToRender, m_spacing);
        for (int s = 0; s < effectiveSpacing; ++s) batchAnsi += "\r\n";
    }

    batchAnsi += renderItemToAnsi(itemToRender, output);
    if (!batchAnsi.empty()) {
        output->print(QString::fromStdString(batchAnsi));
    }
    m_hasRenderedFirstItem = true;
    ++m_consumedRowCount;
}

void StreamView::append(qtui::Item* item) {
    Terminal* output = terminal();
    if (!item || !output) return;

    std::string batchAnsi;
    if (m_hasRenderedFirstItem) {
        if (m_separator) {
            std::string sepAnsi = renderSeparatorToAnsi(output);
            if (!sepAnsi.empty()) batchAnsi += sepAnsi;
        }
        const int effectiveSpacing = resolveItemSpacing(item, m_spacing);
        for (int s = 0; s < effectiveSpacing; ++s) batchAnsi += "\r\n";
    }

    batchAnsi += renderItemToAnsi(item, output);
    if (!batchAnsi.empty()) {
        output->print(QString::fromStdString(batchAnsi));
    }
    m_hasRenderedFirstItem = true;
    ++m_consumedRowCount;
}

void StreamView::print(const QString& text) {
    Terminal* output = terminal();
    if (output) {
        output->print(text);
    }
}

QQmlComponent* StreamView::delegateForRow(int row) const {
    QQmlListReference choices(m_delegate, "choices");
    if (!choices.isValid()) {
        return m_delegate;
    }

    const QModelIndex modelIndex = m_model->index(row, 0);
    const QString roleName = m_delegate->property("role").toString();
    QVariant roleValue;
    const QHash<int, QByteArray> roles = m_model->roleNames();
    for (auto role = roles.cbegin(); role != roles.cend(); ++role) {
        if (role.value() == roleName.toUtf8()) {
            roleValue = m_model->data(modelIndex, role.key());
            break;
        }
    }

    for (qsizetype choiceIndex = 0; choiceIndex < choices.count(); ++choiceIndex) {
        QObject* choice = choices.at(choiceIndex);
        if (!choice) continue;

        const int choiceRow = choice->property("row").toInt();
        const int choiceColumn = choice->property("column").toInt();
        const QVariant choiceValue = choice->property("roleValue");
        const bool rowMatches = choiceRow < 0 || choiceRow == row;
        const bool columnMatches = choiceColumn < 0 || choiceColumn == 0;
        const bool roleMatches = !choiceValue.isValid() || choiceValue == roleValue;
        if (!rowMatches || !columnMatches || !roleMatches) continue;

        const QVariant prop = choice->property("delegate");
        if (auto* comp = prop.value<QQmlComponent*>())
            return comp;
        if (auto* obj = prop.value<QObject*>())
            return qobject_cast<QQmlComponent*>(obj);
    }

    return nullptr;
}

Terminal* StreamView::terminal() const {
    QObject* current = parent();
    while (current) {
        if (auto* result = qobject_cast<Terminal*>(current)) {
            return result;
        }
        current = current->parent();
    }
    return nullptr;
}

} // namespace qtui
