#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QQmlListProperty>
#include "Animation.h"

namespace qtui {

class Transition : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QString to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QQmlListProperty<qtui::Animation> animations READ animations)
    Q_CLASSINFO("DefaultProperty", "animations")

public:
    explicit Transition(QObject* parent = nullptr);
    ~Transition() override = default;

    QString from() const { return m_from; }
    void setFrom(const QString& f);

    QString to() const { return m_to; }
    void setTo(const QString& t);

    QQmlListProperty<Animation> animations();
    const QList<Animation*>& animationsList() const { return m_animationsList; }

    bool matches(const QString& fromState, const QString& toState) const;

signals:
    void fromChanged();
    void toChanged();

private:
    static void appendAnimation(QQmlListProperty<Animation>* list, Animation* anim);
    static qsizetype animationCount(QQmlListProperty<Animation>* list);
    static Animation* animationAt(QQmlListProperty<Animation>* list, qsizetype index);
    static void clearAnimations(QQmlListProperty<Animation>* list);

    QString m_from = "*";
    QString m_to = "*";
    QList<Animation*> m_animationsList;
};

} // namespace qtui
