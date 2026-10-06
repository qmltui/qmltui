#pragma once

#include <Item.h>
#include <Gradient.h>
#include <QString>

namespace qtui {

class Gauge : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY maxValueChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(qtui::Color gaugeColor READ gaugeColor WRITE setGaugeColor NOTIFY gaugeColorChanged)
    Q_PROPERTY(qtui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)
    Q_PROPERTY(qtui::Color bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)
    Q_PROPERTY(QString filledChar READ filledChar WRITE setFilledChar NOTIFY filledCharChanged)
    Q_PROPERTY(QString unfilledChar READ unfilledChar WRITE setUnfilledChar NOTIFY unfilledCharChanged)
    Q_PROPERTY(bool showPercentage READ showPercentage WRITE setShowPercentage NOTIFY showPercentageChanged)
    Q_PROPERTY(bool showBrackets READ showBrackets WRITE setShowBrackets NOTIFY showBracketsChanged)

public:
    explicit Gauge(QObject *parent = nullptr);
    ~Gauge() override = default;

    double value() const { return m_value; }
    void setValue(double val);

    double maxValue() const { return m_maxValue; }
    void setMaxValue(double maxVal);

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    Color gaugeColor() const { return m_gaugeColor; }
    void setGaugeColor(Color color);

    Gradient* gradient() const { return m_gradient; }
    void setGradient(Gradient* gradient);

    Color bgColor() const { return m_bgColor; }
    void setBgColor(Color color);

    QString filledChar() const { return m_filledChar; }
    void setFilledChar(const QString &ch);

    QString unfilledChar() const { return m_unfilledChar; }
    void setUnfilledChar(const QString &ch);

    bool showPercentage() const { return m_showPercentage; }
    void setShowPercentage(bool show);

    bool showBrackets() const { return m_showBrackets; }
    void setShowBrackets(bool show);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void valueChanged();
    void maxValueChanged();
    void labelChanged();
    void gaugeColorChanged();
    void gradientChanged();
    void bgColorChanged();
    void filledCharChanged();
    void unfilledCharChanged();
    void showPercentageChanged();
    void showBracketsChanged();

private:
    double m_value = 0.0;
    double m_maxValue = 100.0;
    QString m_label;

    Color m_gaugeColor = "green";
    uint32_t m_resolvedGaugeColor = 0xFF00FF00;
    Gradient* m_gradient = nullptr;

    Color m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;

    QString m_filledChar = "█";
    QString m_unfilledChar = "░";
    bool m_showPercentage = true;
    bool m_showBrackets = true;
};

} // namespace qtui
