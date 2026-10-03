#include "Loader.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QQmlInfo>

#include <algorithm>

namespace qtui {

namespace {

QString componentErrors(const QQmlComponent *component) {
    if (!component) return {};

    QStringList messages;
    const QList<QQmlError> errors = component->errors();
    messages.reserve(errors.size());
    for (const QQmlError &error : errors)
        messages.append(error.toString());
    return messages.join(QLatin1Char('\n'));
}

} // namespace

Loader::Loader(QObject *parent)
    : Item(parent) {
}

Loader::~Loader() {
    disconnectComponent();
    clearItem();
    clearUrlComponent();
}

void Loader::classBegin() {
    m_complete = false;
}

void Loader::componentComplete() {
    m_complete = true;
    beginLoading();
}

void Loader::setActive(bool active) {
    if (m_active == active) return;

    m_active = active;
    emit activeChanged();

    if (!m_active) {
        clearItem();
        if (m_urlComponent) {
            disconnectComponent();
            clearUrlComponent();
        }
        setProgress(0.0);
        setErrorString({});
        setStatus(Null);
        return;
    }

    beginLoading();
}

void Loader::setSource(const QUrl &source) {
    if (m_source == source && !m_sourceComponent) return;

    clearItem();
    disconnectComponent();
    clearUrlComponent();

    if (m_sourceComponent) {
        m_sourceComponent.clear();
        emit sourceComponentChanged();
    }

    if (m_source != source) {
        m_source = source;
        emit sourceChanged();
    }

    setProgress(0.0);
    setErrorString({});
    setStatus(Null);
    beginLoading();
}

void Loader::setSourceComponent(QQmlComponent *component) {
    if (m_sourceComponent == component && m_source.isEmpty()) return;

    clearItem();
    disconnectComponent();
    clearUrlComponent();

    if (!m_source.isEmpty()) {
        m_source = QUrl();
        emit sourceChanged();
    }

    m_sourceComponent = component;
    emit sourceComponentChanged();

    setProgress(0.0);
    setErrorString({});
    setStatus(Null);

    if (m_sourceComponent)
        watchComponent(m_sourceComponent, true);
    beginLoading();
}

void Loader::setAsynchronous(bool asynchronous) {
    if (m_asynchronous == asynchronous) return;
    m_asynchronous = asynchronous;
    emit asynchronousChanged();
}

void Loader::setWidth(int width) {
    Item::setWidth(width);
    if (m_item && width >= 0)
        m_item->setWidth(width);
}

void Loader::setHeight(int height) {
    Item::setHeight(height);
    if (m_item && height >= 0)
        m_item->setHeight(height);
}

QQmlComponent *Loader::effectiveComponent() const {
    return m_sourceComponent ? m_sourceComponent.data() : m_urlComponent.data();
}

void Loader::beginLoading() {
    if (!m_complete || !m_active) return;

    if (m_sourceComponent) {
        handleComponentStatus();
        return;
    }

    if (m_source.isEmpty()) {
        setStatus(Null);
        return;
    }

    if (m_urlComponent) {
        handleComponentStatus();
        return;
    }

    QQmlEngine *engine = qmlEngine(this);
    if (!engine) {
        setErrorString(QStringLiteral("Loader has no QQmlEngine"));
        setStatus(Error);
        return;
    }

    m_urlComponent = new QQmlComponent(engine, this);
    watchComponent(m_urlComponent, false);

    QUrl resolvedSource = m_source;
    if (QQmlContext *context = qmlContext(this))
        resolvedSource = context->resolvedUrl(resolvedSource);

    m_urlComponent->loadUrl(
        resolvedSource,
        m_asynchronous ? QQmlComponent::Asynchronous
                       : QQmlComponent::PreferSynchronous);
    handleComponentStatus();
}

void Loader::watchComponent(QQmlComponent *component, bool external) {
    disconnectComponent();
    if (!component) return;

    m_componentConnections.append(connect(
        component, &QQmlComponent::statusChanged,
        this, [this](QQmlComponent::Status) { handleComponentStatus(); }));
    m_componentConnections.append(connect(
        component, &QQmlComponent::progressChanged,
        this, [this](qreal) { handleComponentStatus(); }));
    m_componentConnections.append(connect(
        component, &QObject::destroyed,
        this, [this, external]() {
            if (external) {
                m_sourceComponent.clear();
                emit sourceComponentChanged();
            } else {
                m_urlComponent.clear();
            }
            m_componentConnections.clear();
            clearItem();
            setProgress(0.0);
            setErrorString({});
            setStatus(Null);
        }));
}

void Loader::handleComponentStatus() {
    if (!m_active) {
        setStatus(Null);
        return;
    }

    QQmlComponent *component = effectiveComponent();
    if (!component) {
        setProgress(0.0);
        setStatus(Null);
        return;
    }

    setProgress(component->progress());
    switch (component->status()) {
    case QQmlComponent::Null:
        setStatus(Null);
        break;
    case QQmlComponent::Loading:
        setErrorString({});
        setStatus(Loading);
        break;
    case QQmlComponent::Error:
        clearItem();
        setErrorString(componentErrors(component));
        setStatus(Error);
        break;
    case QQmlComponent::Ready:
        createItem();
        break;
    }
}

void Loader::createItem() {
    if (m_item || !m_active) return;

    QQmlComponent *component = effectiveComponent();
    if (!component || !component->isReady()) return;

    QQmlContext *context = component->creationContext();
    if (!context)
        context = qmlContext(this);

    QObject *object = component->create(context);
    Item *loadedItem = qobject_cast<Item *>(object);
    if (!loadedItem) {
        delete object;
        QString error = componentErrors(component);
        if (error.isEmpty())
            error = QStringLiteral("Loader source must create a qtui::Item");
        setErrorString(error);
        qmlWarning(this) << error;
        setStatus(Error);
        return;
    }

    m_item = loadedItem;
    loadedItem->setParentItem(this);
    loadedItem->setX(0);
    loadedItem->setY(0);

    m_itemConnections.append(connect(
        loadedItem, &Item::implicitWidthChanged,
        this, [this]() { syncImplicitSize(); }));
    m_itemConnections.append(connect(
        loadedItem, &Item::implicitHeightChanged,
        this, [this]() { syncImplicitSize(); }));
    m_itemConnections.append(connect(
        loadedItem, &QObject::destroyed,
        this, [this]() {
            m_item.clear();
            m_itemConnections.clear();
            syncImplicitSize();
            emit itemChanged();
            if (m_status == Ready)
                setStatus(Null);
        }));

    syncImplicitSize();
    syncItemSize();
    setErrorString({});
    emit itemChanged();
    setStatus(Ready);
    emit loaded();
}

void Loader::clearItem() {
    if (!m_item) {
        syncImplicitSize();
        return;
    }

    for (const QMetaObject::Connection &connection : std::as_const(m_itemConnections))
        disconnect(connection);
    m_itemConnections.clear();

    Item *oldItem = m_item.data();
    m_item.clear();
    oldItem->setParentItem(nullptr);
    delete oldItem;

    syncImplicitSize();
    emit itemChanged();
}

void Loader::clearUrlComponent() {
    if (!m_urlComponent) return;
    QQmlComponent *component = m_urlComponent.data();
    m_urlComponent.clear();
    delete component;
}

void Loader::disconnectComponent() {
    for (const QMetaObject::Connection &connection : std::as_const(m_componentConnections))
        disconnect(connection);
    m_componentConnections.clear();
}

void Loader::syncImplicitSize() {
    setImplicitWidth(m_item ? m_item->implicitWidth() : 0);
    setImplicitHeight(m_item ? m_item->implicitHeight() : 0);
}

void Loader::syncItemSize() {
    if (!m_item) return;
    if (m_width >= 0)
        m_item->setWidth(m_width);
    if (m_height >= 0)
        m_item->setHeight(m_height);
}

void Loader::setStatus(Status status) {
    if (m_status == status) return;
    m_status = status;
    emit statusChanged();
}

void Loader::setProgress(qreal progress) {
    progress = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_progress, progress)) return;
    m_progress = progress;
    emit progressChanged();
}

void Loader::setErrorString(const QString &errorString) {
    if (m_errorString == errorString) return;
    m_errorString = errorString;
    emit errorStringChanged();
}

} // namespace qtui
