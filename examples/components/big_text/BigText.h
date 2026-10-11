#pragma once

#include <Qamui/Core/items/Item.h>
#include <QString>

namespace qamui {

// Recipe: a custom C++ item that paints banner text. Lives with the example,
// not in the library; copy it into your own QML module.
class BigText : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qamui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)

public:
    explicit BigText(QObject *parent = nullptr);
    ~BigText() override = default;

    QString text() const { return m_text; }
    void setText(const QString &text);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void backgroundColorChanged();

private:
    QString m_text;

    Color m_color = "yellow";
    uint32_t m_resolvedColor = 0xFFFFFF00;

    Color m_backgroundColor = "transparent";
    uint32_t m_resolvedBackgroundColor = 0x00000000;
};

} // namespace qamui
