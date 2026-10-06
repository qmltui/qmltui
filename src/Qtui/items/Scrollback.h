#pragma once

#include <QtuiExport.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlParserStatus>
#include <QVariant>

namespace qtui {

class Item;
class Terminal;

class QTUI_CORE_EXPORT Scrollback : public QObject, public QQmlParserStatus {
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "delegate")

    Q_PROPERTY(QVariant model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(int spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
    Q_PROPERTY(QQmlComponent* separator READ separator WRITE setSeparator NOTIFY separatorChanged)
    Q_PROPERTY(bool reuseItems READ reuseItems WRITE setReuseItems NOTIFY reuseItemsChanged)

public:
    explicit Scrollback(QObject* parent = nullptr);
    ~Scrollback() override;
    void classBegin() override {}
    void componentComplete() override { appendExistingRows(); }

    QVariant model() const { return m_modelVariant; }
    void setModel(const QVariant& model);
    void setModel(QAbstractItemModel* model);

    QQmlComponent* delegate() const { return m_delegate; }
    void setDelegate(QQmlComponent* delegate);

    int spacing() const { return m_spacing; }
    void setSpacing(int spacing);

    QQmlComponent* separator() const { return m_separator; }
    void setSeparator(QQmlComponent* separator);

    bool reuseItems() const { return m_reuseItems; }
    void setReuseItems(bool reuse);

    Q_INVOKABLE void append(const QVariant& data);
    Q_INVOKABLE void append(qtui::Item* item);
    Q_INVOKABLE bool print(const QString& text);
    Q_INVOKABLE void clear();

signals:
    void outputRejected(const QString& reason);
    void modelChanged();
    void delegateChanged();
    void spacingChanged();
    void separatorChanged();
    void reuseItemsChanged();

private:
    struct CachedDelegate {
        QPointer<Item> item;
        QPointer<QQmlContext> context;
    };

    int appendRows(int first, int last);
    void appendExistingRows();
    QQmlComponent* delegateForRow(int row) const;
    Terminal* terminal() const;

    CachedDelegate getOrCreateDelegate(QQmlComponent* component, const QVariantMap& initialProperties);
    CachedDelegate getOrCreateSeparator();
    void clearDelegateCache();
    std::string renderItemToAnsi(Item* item, Terminal* output);
    std::string renderSeparatorToAnsi(Terminal* output);
    void applyModelDataToDelegate(Item* item, QQmlContext* context, QQmlComponent* component,
                                 int index, const QVariantMap& modelMap, const QVariant& rawData);

    QVariant m_modelVariant;
    QPointer<QAbstractItemModel> m_model;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_separator;
    int m_spacing = 0;
    bool m_reuseItems = true;
    int m_consumedRowCount = 0;
    bool m_hasRenderedFirstItem = false;

    QHash<QQmlComponent*, CachedDelegate> m_delegateCache;
    CachedDelegate m_cachedSeparator;
};

} // namespace qtui
