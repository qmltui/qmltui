#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>

namespace qtui {

class QTUI_CORE_EXPORT Row : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    explicit Row(QObject *parent = nullptr);
    ~Row() override;

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    int implicitWidth() const override;
    int implicitHeight() const override;
    void updateLayout() override;
    void layoutChildren();

signals:
    void spacingChanged();

private:
    int m_spacing = 0;
};

} // namespace qtui
