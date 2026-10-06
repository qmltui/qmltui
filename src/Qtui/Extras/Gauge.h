#pragma once

#include <QtuiExport.h>

#include <Item.h>
#include <Gradient.h>
#include <QString>

namespace qtui {

class QTUI_EXTRAS_EXPORT Gauge : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(double to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Gradient* gradient READ gradient WRITE setGradient NOTIFY gradientChanged)
    Q_PROPERTY(qtui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QString fillCharacter READ fillCharacter WRITE setFillCharacter NOTIFY fillCharacterChanged)
    Q_PROPERTY(QString emptyCharacter READ emptyCharacter WRITE setEmptyCharacter NOTIFY emptyCharacterChanged)
    Q_PROPERTY(bool showPercentage READ showPercentage WRITE setShowPercentage NOTIFY showPercentageChanged)
    Q_PROPERTY(bool showBrackets READ showBrackets WRITE setShowBrackets NOTIFY showBracketsChanged)

public:
    explicit Gauge(QObject *parent = nullptr);
    ~Gauge() override = default;

    double value() const { return m_value; }
    void setValue(double val);

    double from() const { return m_from; }
    void setFrom(double from);

    double to() const { return m_to; }
    void setTo(double maxVal);

    QString text() const { return m_text; }
    void setText(const QString &text);

    Color color() const { return m_gaugeColor; }
    void setColor(Color color);

    Gradient* gradient() const { return m_gradient; }
    void setGradient(Gradient* gradient);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    QString fillCharacter() const { return m_filledChar; }
    void setFillCharacter(const QString &ch);

    QString emptyCharacter() const { return m_unfilledChar; }
    void setEmptyCharacter(const QString &ch);

    bool showPercentage() const { return m_showPercentage; }
    void setShowPercentage(bool show);

    bool showBrackets() const { return m_showBrackets; }
    void setShowBrackets(bool show);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void valueChanged();
    void fromChanged();
    void toChanged();
    void textChanged();
    void colorChanged();
    void gradientChanged();
    void backgroundColorChanged();
    void fillCharacterChanged();
    void emptyCharacterChanged();
    void showPercentageChanged();
    void showBracketsChanged();

private:
    double m_value = 0.0;
    double m_from = 0.0;
    double m_to = 100.0;
    QString m_text;

    Color m_gaugeColor = "green";
    uint32_t m_resolvedColor = 0xFF00FF00;
    Gradient* m_gradient = nullptr;

    Color m_backgroundColor = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;

    QString m_filledChar = "█";
    QString m_unfilledChar = "░";
    bool m_showPercentage = true;
    bool m_showBrackets = true;
};

} // namespace qtui
