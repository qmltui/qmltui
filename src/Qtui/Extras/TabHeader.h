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
    Q_PROPERTY(QString activeTabColor READ activeTabColor WRITE setActiveTabColor NOTIFY activeTabColorChanged)
    Q_PROPERTY(QString activeTabBgColor READ activeTabBgColor WRITE setActiveTabBgColor NOTIFY activeTabBgColorChanged)
    Q_PROPERTY(QString inactiveTabColor READ inactiveTabColor WRITE setInactiveTabColor NOTIFY inactiveTabColorChanged)
    Q_PROPERTY(QString inactiveTabBgColor READ inactiveTabBgColor WRITE setInactiveTabBgColor NOTIFY inactiveTabBgColorChanged)

public:
    explicit TabHeader(QObject *parent = nullptr);
    ~TabHeader() override = default;

    QVariant tabs() const { return m_tabsVariant; }
    void setTabs(const QVariant &tabs);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QString activeTabColor() const { return m_activeTabColor; }
    void setActiveTabColor(const QString &color);

    QString activeTabBgColor() const { return m_activeTabBgColor; }
    void setActiveTabBgColor(const QString &color);

    QString inactiveTabColor() const { return m_inactiveTabColor; }
    void setInactiveTabColor(const QString &color);

    QString inactiveTabBgColor() const { return m_inactiveTabBgColor; }
    void setInactiveTabBgColor(const QString &color);

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
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

    QString m_activeTabColor = "white";
    uint32_t m_resolvedActiveTabColor = 0xFFFFFFFF;
    QString m_activeTabBgColor = "blue";
    uint32_t m_resolvedActiveTabBgColor = 0xFF0000FF;

    QString m_inactiveTabColor = "gray";
    uint32_t m_resolvedInactiveTabColor = 0xFF808080;
    QString m_inactiveTabBgColor = "transparent";
    uint32_t m_resolvedInactiveTabBgColor = 0x00000000;

};

} // namespace qtui
