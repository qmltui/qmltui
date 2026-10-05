#pragma once

#include <Item.h>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

#include "MdDoc.h"
#include "MdPalette.h"
#include "MdViewStyle.h"

namespace qtui {

class MdView : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qtui::MdDoc* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
    Q_PROPERTY(int contentHeight READ contentHeight NOTIFY contentHeightChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(int activeMatchIndex READ activeMatchIndex WRITE setActiveMatchIndex NOTIFY activeMatchIndexChanged)

    // View Style & Direct Palette Overrides
    Q_PROPERTY(qtui::MdViewStyle* style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(qtui::MdPalette palette READ palette WRITE setPalette NOTIFY paletteChanged)

    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY showLineNumbersChanged)
    Q_PROPERTY(QString renderStyle READ renderStyle WRITE setRenderStyle NOTIFY renderStyleChanged)

public:
    explicit MdView(QObject *parent = nullptr);
    ~MdView() override = default;

    MdDoc* document() const { return m_document; }
    void setDocument(MdDoc *doc);

    int scrollY() const { return m_scrollY; }
    void setScrollY(int y);

    int contentHeight() const;

    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    int activeMatchIndex() const { return m_activeMatchIndex; }
    void setActiveMatchIndex(int idx);

    MdViewStyle* style() const { return m_style; }
    void setStyle(MdViewStyle *style);

    MdPalette palette() const { return m_palette; }
    void setPalette(const MdPalette &pal);

    MdPalette effectivePalette() const;

    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);

    QString renderStyle() const;
    void setRenderStyle(const QString &style);

    Q_INVOKABLE void scrollToLine(int lineIndex);
    Q_INVOKABLE void scrollBy(int delta);
    Q_INVOKABLE void pageUp();
    Q_INVOKABLE void pageDown();

    void paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect = ClipRect()) override;
    void mousePressEvent(MouseEvent *event) override;
    void wheelEvent(MouseEvent *event) override;

signals:
    void documentChanged();
    void scrollYChanged();
    void contentHeightChanged();
    void searchQueryChanged();
    void activeMatchIndexChanged();
    void styleChanged();
    void paletteChanged();
    void showLineNumbersChanged();
    void renderStyleChanged();
    void linkActivated(const QString &url);

private:
    void updateDimensions();
    void rebuildSearchMatches();
    int viewportHeight() const;
    int lineNumberDigits() const;
    int lineNumberGutterWidth() const;
    void clampScroll();
    QPointer<MdDoc> m_document;
    QPointer<MdViewStyle> m_style;
    int m_scrollY = 0;
    QString m_searchQuery;
    int m_activeMatchIndex = 0;

    MdPalette m_palette;
    bool m_showLineNumbers = false;
    bool m_explicitShowLineNumbers = false;
    bool m_explicitPalette = false;

    QList<SearchMatch> m_searchMatches;
    bool m_updatingDimensions = false;
};

} // namespace qtui
