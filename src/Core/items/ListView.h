#pragma once

#include "ScrollView.h"
#include <QVariant>
#include <QQmlComponent>
#include <QHash>
#include <QMap>
#include <QStack>
#include <QPointer>
#include <QMetaObject>

class QAbstractItemModel;

namespace qtui {

class ListView;

class ListViewAttached : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qtui::ListView* view READ view NOTIFY viewChanged)
    Q_PROPERTY(bool isCurrentItem READ isCurrentItem NOTIFY isCurrentItemChanged)

public:
    explicit ListViewAttached(QObject *parent = nullptr) : QObject(parent) {}
    ListView *view() const;
    bool isCurrentItem() const { return m_isCurrentItem; }

signals:
    void viewChanged();
    void isCurrentItemChanged();
    void pooled();
    void reused();

private:
    friend class ListView;
    void update(ListView *view, bool current);
    QPointer<QObject> m_view;
    bool m_isCurrentItem = false;
};

class ListView : public ScrollView {
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(ListViewAttached)
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString highlightColor READ highlightColor WRITE setHighlightColor NOTIFY highlightColorChanged)
    Q_PROPERTY(QString highlightBgColor READ highlightBgColor WRITE setHighlightBgColor NOTIFY highlightBgColorChanged)
    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged)

public:
    static ListViewAttached *qmlAttachedProperties(QObject *object);

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

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    QString highlightColor() const { return m_highlightColor; }
    void setHighlightColor(const QString& color);

    QString highlightBgColor() const { return m_highlightBgColor; }
    void setHighlightBgColor(const QString& color);

    bool reuseItems() const { return m_reuseItems; }
    void setReuseItems(bool reuse);

    void layoutChildren();
    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;
    int implicitWidth() const override;
    int implicitHeight() const override;

    void keyPressEvent(KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;

signals:
    void modelChanged();
    void delegateChanged();
    void orientationChanged();
    void spacingChanged();
    void currentIndexChanged();
    void highlightColorChanged();
    void highlightBgColorChanged();
    void reuseItemsChanged();
    void itemActivated(int index);

private:
    void disconnectModelSignals();
    Item *createDelegateItem(int index);
    void recycleDelegateItem(Item *item);
    void rebindDelegateItem(Item *item, int index);
    void rebuildViewport(bool force = false);
    void updateContentExtent();
    int delegateStride() const;
    void updateDelegateState(Item *item, int index);
    void refreshDelegateStates();
    void clearItems();
    void generateItems();
    void insertModelRows(QAbstractItemModel *model, int first, int last,
                         const QHash<int, QString> &roleMap);
    void removeModelRows(int first, int last);
    void moveModelRows(int first, int last, int destination);

    QVariant m_model;
    QPointer<QObject> m_objectModel;
    QPointer<QQmlComponent> m_delegate;
    Qt::Orientation m_orientation = Qt::Vertical;
    int m_spacing = 0;
    int m_currentIndex = -1;
    QString m_highlightColor = "black";
    uint32_t m_resolvedHighlightColor = 0xFF000000;
    QString m_highlightBgColor = "white";
    uint32_t m_resolvedHighlightBgColor = 0xFFFFFFFF;
    bool m_reuseItems = true;
    QMap<int, Item*> m_createdItems;
    QStack<Item*> m_reusePool;
    QVariantList m_listValues;
    QHash<int, QString> m_roleMap;
    int m_modelCount = 0;
    int m_delegateExtent = 1;
    bool m_delegateExtentKnown = false;
    int m_materializedFirst = -1;
    int m_materializedLast = -1;
    QList<QMetaObject::Connection> m_modelConnections;
    quint64 m_modelGeneration = 0;
};

} // namespace qtui

QML_DECLARE_TYPEINFO(qtui::ListView, QML_HAS_ATTACHED_PROPERTIES)
