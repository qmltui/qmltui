#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <QObject>
#include <QString>
#include <QQmlEngine>

#include <Qamui/Core/primitives/Color.h>

namespace qamui {

class Item;

class QAMUI_CORE_EXPORT Border : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Border is only available as a grouped property of Rectangle")

public:
    enum Style : uint8_t {
        None = 0,
        Single,
        Double,
        Rounded,
        Ascii,
    };
    Q_ENUM(Style)

    Q_PROPERTY(Style style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(qamui::Color color READ color WRITE setColor NOTIFY colorChanged)

    explicit Border(Item* owner);
    ~Border() override;

    Style style() const { return m_style; }
    void setStyle(Style style);
    void setStyle(const QString& style);

    Color color() const { return m_color; }
    void setColor(Color color);

    static Style styleFromString(QStringView str, Style defaultStyle = None);
    static QString styleToString(Style style);

signals:
    void styleChanged();
    void colorChanged();

private:
    Item* m_owner;
    Style m_style = None;
    Color m_color = "default";
};

} // namespace qamui
