#pragma once

#include "Item.h"

namespace qtui {

class ScrollView : public Item {
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ScrollView)
    QML_ELEMENT

    Q_PROPERTY(int contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)

public:
    explicit ScrollView(QObject *parent = nullptr);
    ~ScrollView() override;

    int contentHeight() const { return m_contentHeight; }
    void setContentHeight(int h);

    int contentY() const { return m_contentY; }
    void setContentY(int y);

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

    void ensureChildVisible(Item* childItem);

signals:
    void contentHeightChanged();
    void contentYChanged();

private:
    int m_contentHeight = 0;
    int m_contentY = 0;
};

} // namespace qtui
