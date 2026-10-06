#pragma once

#include <Item.h>
#include <QVariantList>
#include <QStringList>
#include <QJSValue>

namespace qtui {

class TabHeader : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(qtui::Color activeTabColor READ activeTabColor WRITE setActiveTabColor NOTIFY activeTabColorChanged)
    Q_PROPERTY(qtui::Color activeTabBackgroundColor READ activeTabBackgroundColor WRITE setActiveTabBackgroundColor NOTIFY activeTabBackgroundColorChanged)
    Q_PROPERTY(qtui::Color inactiveTabColor READ inactiveTabColor WRITE setInactiveTabColor NOTIFY inactiveTabColorChanged)
    Q_PROPERTY(qtui::Color inactiveTabBackgroundColor READ inactiveTabBackgroundColor WRITE setInactiveTabBackgroundColor NOTIFY inactiveTabBackgroundColorChanged)

public:
    explicit TabHeader(QObject *parent = nullptr);
    ~TabHeader() override = default;

    QVariant tabs() const { return m_tabsVariant; }
    void setTabs(const QVariant &tabs);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    Color activeTabColor() const { return m_activeTabColor; }
    void setActiveTabColor(Color color);

    Color activeTabBackgroundColor() const { return m_activeTabBackgroundColor; }
    void setActiveTabBackgroundColor(Color color);

    Color inactiveTabColor() const { return m_inactiveTabColor; }
    void setInactiveTabColor(Color color);

    Color inactiveTabBackgroundColor() const { return m_inactiveTabBackgroundColor; }
    void setInactiveTabBackgroundColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void tabsChanged();
    void currentIndexChanged();
    void activeTabColorChanged();
    void activeTabBackgroundColorChanged();
    void inactiveTabColorChanged();
    void inactiveTabBackgroundColorChanged();

private:
    QVariant m_tabsVariant;
    QStringList m_tabList;
    int m_currentIndex = 0;

    Color m_activeTabColor = "white";
    uint32_t m_resolvedActiveTabColor = 0xFFFFFFFF;
    Color m_activeTabBackgroundColor = "blue";
    uint32_t m_resolvedActiveTabBackgroundColor = 0xFF0000FF;

    Color m_inactiveTabColor = "gray";
    uint32_t m_resolvedInactiveTabColor = 0xFF808080;
    Color m_inactiveTabBackgroundColor = "transparent";
    uint32_t m_resolvedInactiveTabBackgroundColor = 0x00000000;
};

} // namespace qtui
