#pragma once

#include <Item.h>
#include <QString>

namespace qtui {

class Gauge : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double maxValue READ maxValue WRITE setMaxValue NOTIFY maxValueChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString gaugeColor READ gaugeColor WRITE setGaugeColor NOTIFY gaugeColorChanged)
    Q_PROPERTY(QString bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)
    Q_PROPERTY(QString filledChar READ filledChar WRITE setFilledChar NOTIFY filledCharChanged)
    Q_PROPERTY(QString unfilledChar READ unfilledChar WRITE setUnfilledChar NOTIFY unfilledCharChanged)
    Q_PROPERTY(bool showPercentage READ showPercentage WRITE setShowPercentage NOTIFY showPercentageChanged)
    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)
    Q_PROPERTY(QString borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

public:
    explicit Gauge(QObject *parent = nullptr);
    ~Gauge() override = default;

    double value() const { return m_value; }
    void setValue(double val);

    double maxValue() const { return m_maxValue; }
    void setMaxValue(double maxVal);

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    QString gaugeColor() const { return m_gaugeColor; }
    void setGaugeColor(const QString &color);

    QString bgColor() const { return m_bgColor; }
    void setBgColor(const QString &color);

    QString filledChar() const { return m_filledChar; }
    void setFilledChar(const QString &ch);

    QString unfilledChar() const { return m_unfilledChar; }
    void setUnfilledChar(const QString &ch);

    bool showPercentage() const { return m_showPercentage; }
    void setShowPercentage(bool show);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

    QString borderColor() const { return m_borderColor; }
    void setBorderColor(const QString &color);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;

signals:
    void valueChanged();
    void maxValueChanged();
    void labelChanged();
    void gaugeColorChanged();
    void bgColorChanged();
    void filledCharChanged();
    void unfilledCharChanged();
    void showPercentageChanged();
    void borderStyleChanged();
    void borderColorChanged();

private:
    double m_value = 0.0;
    double m_maxValue = 100.0;
    QString m_label;

    QString m_gaugeColor = "green";
    uint32_t m_resolvedGaugeColor = 0xFF00FF00;

    QString m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;

    QString m_filledChar = "█";
    QString m_unfilledChar = "░";
    bool m_showPercentage = true;

    QString m_borderStyle = "none";
    QString m_borderColor = "gray";
    uint32_t m_resolvedBorderColor = 0xFF808080;
};

} // namespace qtui
