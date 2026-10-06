#pragma once

#include <QtuiExport.h>

#include <Item.h>
#include <QVariantList>
#include <QString>
#include <QJSValue>

namespace qtui {

class QTUI_EXTRAS_EXPORT Sparkline : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(double from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(double to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(bool filled READ filled WRITE setFilled NOTIFY filledChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit Sparkline(QObject *parent = nullptr);
    ~Sparkline() override = default;

    QVariant values() const { return m_valuesVariant; }
    void setValues(const QVariant &values);

    double from() const { return m_from; }
    void setFrom(double from);

    double to() const { return m_to; }
    void setTo(double to);

    bool filled() const { return m_filled; }
    void setFilled(bool filled);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void valuesChanged();
    void fromChanged();
    void toChanged();
    void filledChanged();
    void colorChanged();
    void backgroundColorChanged();

private:
    QVariant m_valuesVariant;
    QVariantList m_values;

    double m_from = 0.0;
    double m_to = 0.0; // 0.0 means auto-scale
    bool m_filled = false;
    Color m_color = "cyan";
    uint32_t m_resolvedColor = 0xFF00FFFF;

    Color m_backgroundColor = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;
};

} // namespace qtui
