#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <QObject>
#include <QVariantMap>
#include <QQmlEngine>
#include <QQmlListProperty>
#include <QPointer>

namespace qtui {

class QTUI_CORE_EXPORT PropertyChanges : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QVariantMap properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(bool restoreEntryValues READ restoreEntryValues WRITE setRestoreEntryValues NOTIFY restoreEntryValuesChanged)

public:
    explicit PropertyChanges(QObject* parent = nullptr);
    ~PropertyChanges() override = default;

    QObject* target() const { return m_target.data(); }
    void setTarget(QObject* t);

    QVariantMap properties() const { return m_properties; }
    void setProperties(const QVariantMap& p);

    bool restoreEntryValues() const { return m_restoreEntryValues; }
    void setRestoreEntryValues(bool r);

signals:
    void targetChanged();
    void propertiesChanged();
    void restoreEntryValuesChanged();

private:
    QPointer<QObject> m_target;
    QVariantMap m_properties;
    bool m_restoreEntryValues = true;
};

class QTUI_CORE_EXPORT State : public QObject {
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

    QQmlListProperty<QObject> changes();
    const QList<QObject*>& changesList() const { return m_changesList; }

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
    QList<QObject*> m_changesList;
};

} // namespace qtui
