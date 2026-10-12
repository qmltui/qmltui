#pragma once

#include <Qamui/Core/Primitives/QamuiExport.h>

#include <Qamui/Core/Items/ScrollView.h>
#include <QVariant>
#include <QPersistentModelIndex>
#include <QQmlComponent>
#include <QHash>
#include <QMap>
#include <QStack>
#include <QPointer>
#include <QMetaObject>
#include <Qamui/Core/Primitives/ColorMapper.h>

class QAbstractItemModel;

namespace qamui {

class ListView;

class QAMUI_CORE_EXPORT ListViewAttached : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(qamui::ListView* view READ view NOTIFY viewChanged)
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

class QAMUI_CORE_EXPORT ListView : public ScrollView {
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(ListViewAttached)
    Q_CLASSINFO("DefaultProperty", "delegate")
    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(qamui::Color highlightColor READ highlightColor WRITE setHighlightColor NOTIFY highlightColorChanged)
    Q_PROPERTY(qamui::Color highlightBackgroundColor READ highlightBackgroundColor WRITE setHighlightBackgroundColor NOTIFY highlightBackgroundColorChanged)
    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged)
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)

public:
    static ListViewAttached *qmlAttachedProperties(QObject *object);

    explicit ListView(QObject* parent = nullptr);
    ~ListView() override;

    QVariant model() const { return m_model; }
    void setModel(const QVariant& model);

    // Parent index whose children the view shows; invalid means the top level,
    // as with DelegateModel.rootIndex in Qt Quick.
    QModelIndex rootIndex() const { return m_rootIndex; }
    void setRootIndex(const QModelIndex &rootIndex);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation orientation);

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    Color highlightColor() const { return m_highlightColor; }
    void setHighlightColor(Color color);

    Color highlightBackgroundColor() const { return m_highlightBackgroundColor; }
    void setHighlightBackgroundColor(Color color);

    bool reuseItems() const { return m_reuseItems; }
    void setReuseItems(bool reuse);

    void layoutChildren();
    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;
    int implicitWidth() const override;
    int implicitHeight() const override;

    void keyPressEvent(KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;

signals:
    void modelChanged();
    void rootIndexChanged();
    void delegateChanged();
    void orientationChanged();
    void spacingChanged();
    void currentIndexChanged();
    void highlightColorChanged();
    void highlightBackgroundColorChanged();
    void reuseItemsChanged();
    void itemActivated(int index);

protected:
    // Materialises the visible delegate window and positions the items. Runs
    // in the layout pass, never from paint().
    void updateLayout() override;

private:
    void disconnectModelSignals();
    Item *createDelegateItem(int index);
    void recycleDelegateItem(Item *item);
    void rebindDelegateItem(Item *item, int index);
    void rebuildViewport(bool force = false);
    void maybeFetchMore();
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
    int m_pendingCurrentIndex = -1;
    Color m_highlightColor = "default";
    // Resolved caches mirror the Color members: an ANSI-named default must not be
    // written as literal RGB, or the terminal theme stops applying to it (setters
    // early-return when the color is unchanged, so the initializer is the only chance).
    uint32_t m_resolvedHighlightColor = ColorMapper::fromString(m_highlightColor, 0);
    Color m_highlightBackgroundColor = "default";
    uint32_t m_resolvedHighlightBackgroundColor = ColorMapper::fromString(m_highlightBackgroundColor, 0);
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
    QPersistentModelIndex m_rootIndex;
    bool m_nestedWarned = false;
    bool m_fetching = false;
};

} // namespace qamui

QML_DECLARE_TYPEINFO(qamui::ListView, QML_HAS_ATTACHED_PROPERTIES)
