#pragma once

#include <QObject>
#include <QVariant>
#include <QPointer>
#include <QQmlEngine>
#include <QTimer>
#include <QElapsedTimer>
#include <QList>

namespace qtui {

class Easing : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Easing is only available as an enum")
public:
    enum Type {
        Linear,
        InQuad,
        OutQuad,
        InOutQuad,
        InCubic,
        OutCubic,
        InOutCubic,
        OutBounce
    };
    Q_ENUM(Type)
};

class Animation : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Animation is an abstract base class")

    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int loops READ loops WRITE setLoops NOTIFY loopsChanged)

public:
    explicit Animation(QObject* parent = nullptr);
    ~Animation() override;

    bool running() const { return m_running; }
    void setRunning(bool r);

    int loops() const { return m_loops; }
    void setLoops(int l);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();

    virtual void tick(qint64 elapsedMs) = 0;

signals:
    void runningChanged();
    void loopsChanged();
    void started();
    void stopped();
    void finished();

protected:
    bool m_running = false;
    int m_loops = 1;
};

class PropertyAnimation : public Animation {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(QVariant from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(QVariant to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(qtui::Easing::Type easing READ easing WRITE setEasing NOTIFY easingChanged)

public:
    explicit PropertyAnimation(QObject* parent = nullptr);
    ~PropertyAnimation() override = default;

    QObject* target() const { return m_target.data(); }
    void setTarget(QObject* t);

    QString property() const { return m_propertyName; }
    void setProperty(const QString& name);

    int duration() const { return m_duration; }
    void setDuration(int d);

    QVariant from() const { return m_from; }
    void setFrom(const QVariant& f);

    QVariant to() const { return m_to; }
    void setTo(const QVariant& t);

    Easing::Type easing() const { return m_easing; }
    void setEasing(Easing::Type e);

    void tick(qint64 elapsedMs) override;

signals:
    void targetChanged();
    void propertyChanged();
    void durationChanged();
    void fromChanged();
    void toChanged();
    void easingChanged();

protected:
    virtual void writeValue(const QVariant& value);

    QPointer<QObject> m_target;
    QString m_propertyName;
    int m_duration = 250;
    QVariant m_from;
    QVariant m_to;
    Easing::Type m_easing = Easing::Linear;
    QElapsedTimer m_timer;
    qint64 m_accumulatedTime = 0;
};

class NumberAnimation : public PropertyAnimation {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit NumberAnimation(QObject* parent = nullptr);
    ~NumberAnimation() override = default;
};

class ColorAnimation : public PropertyAnimation {
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ColorAnimation(QObject* parent = nullptr);
    ~ColorAnimation() override = default;

protected:
    void writeValue(const QVariant& value) override;
};

class AnimationManager : public QObject {
    Q_OBJECT
public:
    static AnimationManager* instance();

    void registerAnimation(Animation* anim);
    void unregisterAnimation(Animation* anim);

private slots:
    void handleTick();

private:
    explicit AnimationManager(QObject* parent = nullptr);
    ~AnimationManager() override = default;

    QTimer m_timer;
    QList<Animation*> m_animations;
    qint64 m_lastTickTime = 0;
    QElapsedTimer m_time;
};

} // namespace qtui
