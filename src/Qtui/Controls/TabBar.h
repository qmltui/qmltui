#pragma once

#include <QtuiExport.h>

#include <Item.h>
#include <QVariantList>
#include <QStringList>
#include <QJSValue>

namespace qtui {

// A row of tabs navigated with Left/Right or the mouse. Colors come from the
// Palette: the active tab uses highlight/highlightedText, the others windowText.
class QTUI_CONTROLS_EXPORT TabBar : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariant tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)

public:
    explicit TabBar(QObject *parent = nullptr);
    ~TabBar() override = default;

    QVariant tabs() const { return m_tabsVariant; }
    void setTabs(const QVariant &tabs);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) const override;
    void keyPressEvent(KeyEvent *event) override;
    void mousePressEvent(MouseEvent *event) override;

signals:
    void tabsChanged();
    void currentIndexChanged();

private:
    QVariant m_tabsVariant;
    QStringList m_tabList;
    int m_currentIndex = 0;
};

} // namespace qtui
