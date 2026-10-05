#pragma once
#include <Rectangle.h>

namespace qtui {

class ProgressBar : public Rectangle {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(qtui::Color barColor READ barColor WRITE setBarColor NOTIFY barColorChanged)

public:
    explicit ProgressBar(QObject *parent = nullptr);
    ~ProgressBar() override = default;

    double value() const { return m_value; }
    void setValue(double val);

    Color barColor() const { return m_barColor; }
    void setBarColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;

signals:
    void valueChanged();
    void barColorChanged();

private:
    double m_value = 0.0;
    Color m_barColor = "green";
    uint32_t m_resolvedBarColor = 0xFF00FF00;
};

} // namespace qtui
