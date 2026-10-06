#pragma once

#include <Item.h>
#include <QVariantList>
#include <QString>
#include <QJSValue>

namespace qtui {

class Sparkline : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(double minVal READ minVal WRITE setMinVal NOTIFY minValChanged)
    Q_PROPERTY(double maxVal READ maxVal WRITE setMaxVal NOTIFY maxValChanged)
    Q_PROPERTY(bool filled READ filled WRITE setFilled NOTIFY filledChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color bgColor READ bgColor WRITE setBgColor NOTIFY bgColorChanged)

public:
    explicit Sparkline(QObject *parent = nullptr);
    ~Sparkline() override = default;

    QVariant values() const { return m_valuesVariant; }
    void setValues(const QVariant &values);

    double minVal() const { return m_minVal; }
    void setMinVal(double minVal);

    double maxVal() const { return m_maxVal; }
    void setMaxVal(double maxVal);

    bool filled() const { return m_filled; }
    void setFilled(bool filled);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color bgColor() const { return m_bgColor; }
    void setBgColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void valuesChanged();
    void minValChanged();
    void maxValChanged();
    void filledChanged();
    void colorChanged();
    void bgColorChanged();

private:
    QVariant m_valuesVariant;
    QVariantList m_values;

    double m_minVal = 0.0;
    double m_maxVal = 0.0; // 0.0 means auto-scale
    bool m_filled = false;
    Color m_color = "cyan";
    uint32_t m_resolvedColor = 0xFF00FFFF;

    Color m_bgColor = "transparent";
    uint32_t m_resolvedBgColor = 0x00000000;
};

} // namespace qtui
