#pragma once

#include "ScrollView.h"
#include <QVariant>
#include <QQmlComponent>

namespace qtui {

class ListView : public ScrollView {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)

public:
    ListView(QObject* parent = nullptr);
    ~ListView() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    void layoutChildren();
    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;
    int implicitWidth() const override;
    int implicitHeight() const override;

signals:
    void modelChanged();
    void delegateChanged();
    void orientationChanged();
    void spacingChanged();

private:
    void clearItems();
    void generateItems();

    QVariant m_model;
    QQmlComponent* m_delegate = nullptr;
    Qt::Orientation m_orientation = Qt::Vertical;
    int m_spacing = 0;
    QList<Item*> m_createdItems;
};

} // namespace qtui
