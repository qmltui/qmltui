#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <QVariantList>
#include <QStringList>
#include <QJSValue>

namespace qtui {

// A row of tabs navigated with Left/Right or the mouse. The selected tab
// uses reverse video unless explicit selection colors are supplied.
class QTUI_CONTROLS_EXPORT TabBar : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qtui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY selectionColorChanged)
    Q_PROPERTY(qtui::Color selectedTextColor READ selectedTextColor WRITE setSelectedTextColor NOTIFY selectedTextColorChanged)
    Q_PROPERTY(qtui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qtui::Color textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)

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

    Color textColor() const { return m_textColor; }
    void setTextColor(Color color);

    Color color() const { return m_color; }
    void setColor(Color color);

    Color selectedTextColor() const { return m_selectedTextColor; }
    void setSelectedTextColor(Color color);

    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);

signals:
    void selectionColorChanged();
    void selectedTextColorChanged();
    void colorChanged();
    void textColorChanged();
    void tabsChanged();
    void currentIndexChanged();

private:
    Color m_selectionColor;
    Color m_selectedTextColor;
    Color m_color = "default";
    Color m_textColor = "default";
    QVariant m_tabsVariant;
    QStringList m_tabList;
    int m_currentIndex = 0;
};

} // namespace qtui
