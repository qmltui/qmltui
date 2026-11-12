#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>

#include <QList>
#include <QMetaObject>
#include <QPointer>
#include <cstdint>
#include <QUrl>
#include <QQmlComponent>
#include <QQmlParserStatus>

namespace qmltui {

// Dynamically instantiates a QML component from a URL (`source`) or inline
// `Component` (`sourceComponent`) when `active` is true, mirroring its implicit
// size or resizing the loaded item to fill explicit Loader dimensions.
class QMLTUI_CORE_EXPORT Loader : public Item, public QQmlParserStatus {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Loader)
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QQmlComponent* sourceComponent READ sourceComponent WRITE setSourceComponent NOTIFY
                   sourceComponentChanged)
    Q_PROPERTY(QObject* item READ item NOTIFY itemChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(bool asynchronous READ asynchronous WRITE setAsynchronous NOTIFY asynchronousChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

public:
    enum Status : uint8_t {
        Null = QQmlComponent::Null,
        Ready = QQmlComponent::Ready,
        Loading = QQmlComponent::Loading,
        Error = QQmlComponent::Error,
    };
    Q_ENUM(Status)

    explicit Loader(QObject* parent = nullptr);
    ~Loader() override;

    bool active() const { return m_active; }
    void setActive(bool active);

    QUrl source() const { return m_source; }
    void setSource(const QUrl& source);

    QQmlComponent* sourceComponent() const { return m_sourceComponent; }
    void setSourceComponent(QQmlComponent* component);

    QObject* item() const { return m_item; }
    Status status() const { return m_status; }
    qreal progress() const { return m_progress; }

    bool asynchronous() const { return m_asynchronous; }
    void setAsynchronous(bool asynchronous);

    QString errorString() const { return m_errorString; }

    void setWidth(int width) override;
    void setHeight(int height) override;

    void classBegin() override;
    void componentComplete() override;

signals:
    void activeChanged();
    void sourceChanged();
    void sourceComponentChanged();
    void itemChanged();
    void statusChanged();
    void progressChanged();
    void asynchronousChanged();
    void errorStringChanged();
    void loaded();

private:
    QQmlComponent* effectiveComponent() const;
    void beginLoading();
    void watchComponent(QQmlComponent* component, bool external);
    void handleComponentStatus();
    void createItem();
    void clearItem();
    void clearUrlComponent();
    void disconnectComponent();
    Item* loadedVisualItem() const;
    void syncImplicitSize();
    void syncItemSize();
    void setStatus(Status status);
    void setProgress(qreal progress);
    void setErrorString(const QString& errorString);

    bool m_active = true;
    bool m_asynchronous = false;
    bool m_complete = true;
    QUrl m_source;
    QPointer<QQmlComponent> m_sourceComponent;
    QPointer<QQmlComponent> m_urlComponent;
    QPointer<QObject> m_item;
    Status m_status = Null;
    qreal m_progress = 0.0;
    QString m_errorString;
    QList<QMetaObject::Connection> m_componentConnections;
    QList<QMetaObject::Connection> m_itemConnections;
};

} // namespace qmltui
