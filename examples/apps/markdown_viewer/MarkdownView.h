#pragma once

#include <Item.h>
#include <QPointer>
#include <QtQml/qqmlregistration.h>
#include "MarkdownDocument.h"
#include "MarkdownPalette.h"

namespace qtui {

class MarkdownView : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qtui::MarkdownDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
    Q_PROPERTY(int contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int activeMatchIndex READ activeMatchIndex WRITE setActiveMatchIndex NOTIFY activeMatchIndexChanged)

    // Grouped Palette Property (Q_GADGET value type)
    Q_PROPERTY(qtui::MarkdownPalette palette READ palette WRITE setPalette NOTIFY paletteChanged)

    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)

public:
    explicit MarkdownView(QObject *parent = nullptr);
    ~MarkdownView() override = default;

    MarkdownDocument* document() const { return m_document; }
    void setDocument(MarkdownDocument *doc);

    int scrollY() const { return m_scrollY; }
    void setScrollY(int y);

    int contentHeight() const;

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    int activeMatchIndex() const { return m_activeMatchIndex; }
    void setActiveMatchIndex(int idx);

    MarkdownPalette palette() const { return m_palette; }
    void setPalette(const MarkdownPalette &pal);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

    Q_INVOKABLE void scrollToLine(int lineIndex);
    Q_INVOKABLE void scrollBy(int delta);
    Q_INVOKABLE void pageUp();
    Q_INVOKABLE void pageDown();

    void render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void mousePressEvent(MouseEvent *event) override;
    void wheelEvent(MouseEvent *event) override;

signals:
    void documentChanged();
    void scrollYChanged();
    void contentHeightChanged();
    void searchQueryChanged();
    void activeMatchIndexChanged();
    void paletteChanged();
    void borderStyleChanged();
    void linkActivated(const QString &url);

private:
    void updateDimensions();
    void rebuildSearchMatches();
    int viewportHeight() const;
    int lineNumberDigits() const;
    int lineNumberGutterWidth() const;
    void clampScroll();
    void drawBorder(TextGridBuffer &buffer, int absX, int absY,
                    const ClipRect &clipRect) const;

    QPointer<MarkdownDocument> m_document;
    int m_scrollY = 0;
    QString m_searchQuery;
    int m_activeMatchIndex = 0;

    MarkdownPalette m_palette;
    QString m_borderStyle = "none";
    QList<SearchMatch> m_searchMatches;
    bool m_updatingDimensions = false;
};

} // namespace qtui
