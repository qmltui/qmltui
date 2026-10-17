#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QVariantMap>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QPointer>
#include <memory>

namespace qmltui {

class PropertyChangesPrivate;

// Declarative property overrides applied when the enclosing State is active.
//
// With QMLTUI_ENABLE_PRIVATE_QT=ON, direct QML property assignments on
// PropertyChanges are captured by PropertyChangesParser (bound unless
// `explicit: true`). Without Private Qt, overrides are supplied through the
// `properties` variant map.
class QMLTUI_CORE_EXPORT PropertyChanges : public QObject {
    Q_OBJECT
    QML_ELEMENT

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    QML_CUSTOMPARSER
    Q_CLASSINFO("ImmediatePropertyNames", "target,restoreEntryValues,explicit,properties,objectName")
    // When true, expressions on PropertyChanges are evaluated once on state
    // entry instead of establishing live bindings.
    Q_PROPERTY(bool explicit READ isExplicit WRITE setIsExplicit NOTIFY explicitChanged)
#endif

    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QVariantMap properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    // When false, modified properties keep their overridden values after
    // leaving the state instead of reverting to their base values.
    Q_PROPERTY(bool restoreEntryValues READ restoreEntryValues WRITE setRestoreEntryValues NOTIFY restoreEntryValuesChanged)

public:
    explicit PropertyChanges(QObject* parent = nullptr);
    ~PropertyChanges() override;

    QObject* target() const { return m_target.data(); }
    void setTarget(QObject* t);

    QVariantMap properties() const { return m_properties; }
    void setProperties(const QVariantMap& p);

    bool restoreEntryValues() const { return m_restoreEntryValues; }
    void setRestoreEntryValues(bool r);

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    bool isExplicit() const;
    void setIsExplicit(bool e);
#endif

    PropertyChangesPrivate* d_func() { return d.get(); }
    const PropertyChangesPrivate* d_func() const { return d.get(); }

signals:
    void targetChanged();
    void propertiesChanged();
    void restoreEntryValuesChanged();
#if defined(QMLTUI_ENABLE_PRIVATE_QT)
    void explicitChanged();
#endif

private:
    QPointer<QObject> m_target;
    QVariantMap m_properties;
    bool m_restoreEntryValues = true;
    std::unique_ptr<PropertyChangesPrivate> d;
};

// Named set of PropertyChanges belonging to an Item or StateGroup. Activated
// either by assigning its name to `state` or automatically when `when` becomes
// true.
class QMLTUI_CORE_EXPORT State : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(QQmlListProperty<QObject> changes READ changes)
    Q_CLASSINFO("DefaultProperty", "changes")

public:
    explicit State(QObject* parent = nullptr);
    ~State() override = default;

    QString name() const { return m_name; }
    void setName(const QString& n);

    bool when() const { return m_when; }
    void setWhen(bool w);

    QQmlListProperty<QObject> changes() const;
    const QList<QPointer<QObject>>& changesList() const { return m_changesList; }

    QList<PropertyChanges*> propertyChangesList() const;

signals:
    void nameChanged();
    void whenChanged();

private:
    static void appendChange(QQmlListProperty<QObject>* list, QObject* obj);
    static qsizetype changeCount(QQmlListProperty<QObject>* list);
    static QObject* changeAt(QQmlListProperty<QObject>* list, qsizetype index);
    static void clearChanges(QQmlListProperty<QObject>* list);

    QString m_name;
    bool m_when = false;
    QList<QPointer<QObject>> m_changesList;
};

} // namespace qmltui

#if defined(QMLTUI_ENABLE_PRIVATE_QT)
QT_BEGIN_NAMESPACE
class QQmlCustomParser;
template<>
QMLTUI_CORE_EXPORT QQmlCustomParser *qmlCreateCustomParser<qmltui::PropertyChanges>();
QT_END_NAMESPACE
#endif
