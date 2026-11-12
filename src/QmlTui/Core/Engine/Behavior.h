#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QObject>
#include <QQmlEngine>
#include <QtQml/private/qqmlpropertyvalueinterceptor_p.h>
#include <QQmlProperty>
#include <QPointer>
#include <QmlTui/Core/Engine/Animation.h>

namespace qmltui {

// QML property value interceptor (`Behavior on <prop>`) that runs a child
// Animation whenever the intercepted property is written. Available when built
// with QMLTUI_ENABLE_PRIVATE_QT=ON.
class QMLTUI_CORE_EXPORT Behavior : public QObject, public QQmlPropertyValueInterceptor {
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueInterceptor)
    QML_ELEMENT

    Q_PROPERTY(
        qmltui::Animation* animation READ animation WRITE setAnimation NOTIFY animationChanged)
    Q_CLASSINFO("DefaultProperty", "animation")

public:
    explicit Behavior(QObject* parent = nullptr);
    ~Behavior() override = default;

    Animation* animation() const { return m_animation; }
    void setAnimation(Animation* anim);

    // QQmlPropertyValueInterceptor interface
    void setTarget(const QQmlProperty& property) override;
    void write(const QVariant& value) override;

signals:
    void animationChanged();

private:
    QQmlProperty m_property;
    QPointer<Animation> m_animation;
};

} // namespace qmltui
