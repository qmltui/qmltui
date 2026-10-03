#pragma once

#include "Item.h"

namespace qtui {

class Row : public Item {
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

    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void spacingChanged();

private:
    void layoutChildren();

    int m_spacing = 0;
};

} // namespace qtui
