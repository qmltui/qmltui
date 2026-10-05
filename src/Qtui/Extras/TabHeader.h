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
    Q_PROPERTY(qtui::Color activeTabBgColor READ activeTabBgColor WRITE setActiveTabBgColor NOTIFY activeTabBgColorChanged)
    Q_PROPERTY(qtui::Color inactiveTabColor READ inactiveTabColor WRITE setInactiveTabColor NOTIFY inactiveTabColorChanged)
    Q_PROPERTY(qtui::Color inactiveTabBgColor READ inactiveTabBgColor WRITE setInactiveTabBgColor NOTIFY inactiveTabBgColorChanged)

public:
    explicit TabHeader(QObject *parent = nullptr);
    ~TabHeader() override = default;

    QVariant tabs() const { return m_tabsVariant; }
    void setTabs(const QVariant &tabs);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    Color activeTabColor() const { return m_activeTabColor; }
    void setActiveTabColor(Color color);

    Color activeTabBgColor() const { return m_activeTabBgColor; }
    void setActiveTabBgColor(Color color);

    Color inactiveTabColor() const { return m_inactiveTabColor; }
    void setInactiveTabColor(Color color);

    Color inactiveTabBgColor() const { return m_inactiveTabBgColor; }
    void setInactiveTabBgColor(Color color);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void tabsChanged();
    void currentIndexChanged();
    void activeTabColorChanged();
    void activeTabBgColorChanged();
    void inactiveTabColorChanged();
    void inactiveTabBgColorChanged();

private:
    QVariant m_tabsVariant;
    QStringList m_tabList;
    int m_currentIndex = 0;

    Color m_activeTabColor = "white";
    uint32_t m_resolvedActiveTabColor = 0xFFFFFFFF;
    Color m_activeTabBgColor = "blue";
    uint32_t m_resolvedActiveTabBgColor = 0xFF0000FF;

    Color m_inactiveTabColor = "gray";
    uint32_t m_resolvedInactiveTabColor = 0xFF808080;
    Color m_inactiveTabBgColor = "transparent";
    uint32_t m_resolvedInactiveTabBgColor = 0x00000000;
};

} // namespace qtui
