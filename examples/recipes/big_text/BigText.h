#pragma once

#include <QmlTui/Core/Items/Item.h>
#include <QString>

namespace qmltui {

// Recipe: a custom C++ item that paints banner text. Lives with the example,
// not in the library; copy it into your own QML module.
class BigText : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qmltui::Color backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)

public:
    explicit BigText(QObject* parent = nullptr);
    ~BigText() override = default;

    QString text() const { return m_text; }
    void setText(const QString& text);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(Color color);

    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void backgroundColorChanged();

private:
    QString m_text;

    Color m_color = Color::Default;
    uint32_t m_resolvedColor = Color::DefaultTag;

    Color m_backgroundColor = Color::Transparent;
    uint32_t m_resolvedBackgroundColor = Color::TransparentTag;
};

} // namespace qmltui
