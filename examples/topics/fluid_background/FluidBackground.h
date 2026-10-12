#pragma once

#include <Qamui/Core/items/Item.h>
#include <Qamui/Core/primitives/ColorMapper.h>
#include <QList>
#include <QString>

namespace qamui {

class ClockTimer;

struct FluidRipple {
    float x = 0.5f; // Normalized [0, 1]
    float y = 0.5f; // Normalized [0, 1]
    float intensity = 1.0f;
    float radius = 0.05f;
    float maxRadius = 0.6f;
    float speed = 0.35f;
};

class FluidBackground : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(qreal time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(QString charset READ charset WRITE setCharset NOTIFY charsetChanged)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qamui::Color secondaryColor READ secondaryColor WRITE setSecondaryColor NOTIFY
                   secondaryColorChanged)
    Q_PROPERTY(qamui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(bool autoWakes READ autoWakes WRITE setAutoWakes NOTIFY autoWakesChanged)
    Q_PROPERTY(int colorMode READ colorMode WRITE setColorMode NOTIFY colorModeChanged)

public:
    enum ColorMode { Accent = 0, Gradient = 1, Monochrome = 2 };
    Q_ENUM(ColorMode)

    explicit FluidBackground(QObject* parent = nullptr);
    ~FluidBackground() override;

    bool running() const { return m_running; }
    void setRunning(bool r);

    qreal speed() const { return m_speed; }
    void setSpeed(qreal s);

    qreal time() const { return m_time; }
    void setTime(qreal t);

    QString charset() const { return m_charset; }
    void setCharset(const QString& cs);

    Color color() const { return m_color; }
    void setColor(Color c);

    Color secondaryColor() const { return m_secondaryColor; }
    void setSecondaryColor(Color c);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color c);

    bool interactive() const { return m_interactive; }
    void setInteractive(bool i);

    bool autoWakes() const { return m_autoWakes; }
    void setAutoWakes(bool a);

    int colorMode() const { return m_colorMode; }
    void setColorMode(int m);

    Q_INVOKABLE void addWake(float normX, float normY, float strength = 1.0f);
    Q_INVOKABLE void clearWakes();

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;
    void mousePressEvent(class MouseEvent* event) override;
    void mouseMoveEvent(class MouseEvent* event) override;

signals:
    void runningChanged();
    void speedChanged();
    void timeChanged();
    void charsetChanged();
    void colorChanged();
    void secondaryColorChanged();
    void backgroundColorChanged();
    void interactiveChanged();
    void autoWakesChanged();
    void colorModeChanged();

private slots:
    void onTick();

private:
    void updateTimer();
    uint32_t evaluateColor(float light) const;

    bool m_running = true;
    qreal m_speed = 1.0;
    qreal m_time = 0.0;
    QString m_charset = QStringLiteral(" ·:;=+*#%@");

    Color m_color = QStringLiteral("#38bdf8"); // Sky cyan
    uint32_t m_resolvedColor = 0xFF38BDF8;

    Color m_secondaryColor = QStringLiteral("#ec4899"); // Hot pink
    uint32_t m_resolvedSecondaryColor = 0xFFEC4899;

    Color m_backgroundColor = QStringLiteral("#080c14"); // Dark night
    uint32_t m_resolvedBgColor = 0xFF080C14;

    bool m_interactive = true;
    bool m_autoWakes = true;
    int m_colorMode = Gradient;

    ClockTimer* m_timer = nullptr;
    QList<FluidRipple> m_ripples;
    float m_autoWakeTimer = 0.0f;
};

} // namespace qamui
