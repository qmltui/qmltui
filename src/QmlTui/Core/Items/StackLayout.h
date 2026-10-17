#pragma once

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Primitives/QmlTuiExport.h>

namespace qmltui {

// Layout that shows only the child item at `currentIndex`, sizing that active
// child to fill the layout's padded content area.
class QMLTUI_CORE_EXPORT StackLayout : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit StackLayout(QObject* parent = nullptr);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    int count() const { return static_cast<int>(childItems().size()); }

    int implicitWidth() const override;
    int implicitHeight() const override;
    void updateLayout() override;
    void layoutChildren();

signals:
    void currentIndexChanged();
    void countChanged();

private:
    int m_currentIndex = 0;
    int m_lastCount = 0;
};

} // namespace qmltui
