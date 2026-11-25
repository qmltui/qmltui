#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QmlTui/Core/Primitives/Span.h>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlParserStatus>
#include <QVariant>
#include <memory>

namespace qmltui {

class Item;
class Terminal;
class TextGridBuffer;

// Appends rendered delegate items or plain text rows into the host terminal's
// native scrollback buffer above the live `Terminal.Inline` viewport.
class QMLTUI_CORE_EXPORT Scrollback : public QObject, public QQmlParserStatus {
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
    void componentComplete() override;

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

    // Renders a single model payload through `delegate` (or a standalone Item
    // or styled Span run) and commits its cells to scrollback. Emits
    // `outputRejected` if the session queue limit is exceeded or the item
    // cannot be rendered (in AltScreen mode, output stays queued until Inline
    // resumes or shutdown).
    Q_INVOKABLE void append(const QVariant& data);
    void append(qmltui::Item* item);
    void append(const qmltui::Span& span);
    void append(const QList<qmltui::Span>& spans);
    Q_INVOKABLE bool print(const QVariant& value);
    bool print(const QString& text);
    bool print(const qmltui::Span& span);
    bool print(const QList<qmltui::Span>& spans);
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
        bool usesTerminalWidth = false;
    };

    int appendRows(int first, int last);
    void appendExistingRows();
    QQmlComponent* delegateForRow(int row) const;
    Terminal* terminal() const;

    CachedDelegate getOrCreateDelegate(QQmlComponent* component,
                                       const QVariantMap& initialProperties);
    CachedDelegate getOrCreateSeparator();
    void clearDelegateCache();
    // Separator (after the first item), spacing rows and the item are painted
    // into one buffer and submitted as one block, so the engine can keep the
    // cells alongside the bytes it sends.
    bool submitBatch(Item* item, Terminal* output, bool usesTerminalWidth);
    Item* separatorItem(Terminal* output, std::unique_ptr<Item>& owned, bool& outUsesTerminalWidth);
    static int resolvedWidth(Item* item, Terminal* output, bool usesTerminalWidth);
    static int resolvedHeight(Item* item);
    bool reportRejection(const QString& reason);
    void applyModelDataToDelegate(Item* item, QQmlContext* context, QQmlComponent* component,
                                  int index, const QVariantMap& modelMap, const QVariant& rawData);

    QVariant m_modelVariant;
    QPointer<QAbstractItemModel> m_model;
    QPointer<QQmlComponent> m_delegate;
    QPointer<QQmlComponent> m_separator;
    int m_spacing = 0;
    bool m_reuseItems = true;
    bool m_fetching = false;
    int m_consumedRowCount = 0;
    bool m_hasRenderedFirstItem = false;

    QHash<QQmlComponent*, CachedDelegate> m_delegateCache;
    CachedDelegate m_cachedSeparator;
};

} // namespace qmltui
