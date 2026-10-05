#pragma once

#include "Item.h"

namespace qtui {

class Column : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    explicit Column(QObject *parent = nullptr);
    ~Column() override;

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
