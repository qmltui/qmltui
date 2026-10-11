#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace qamui {

class MdDocStyle : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Heading1Format {
        Badge,    // Inverted background pill
        Banner,   // Decorative underline banner
        Plain     // Standard text
    };
    Q_ENUM(Heading1Format)

    enum class HeadingFormat {
        Hashes,   // ## Heading
        Bullets,  // ■ Heading, ● Heading, ◆ Heading...
        Plain     // Plain text heading
    };
    Q_ENUM(HeadingFormat)

    enum class CodeBlockFormat {
        Indented,   // Indented borderless syntax block
        Boxed,      // ┌─ [ lang ] ─┐ ... └─┘
        Borderless  // Unindented borderless
    };
    Q_ENUM(CodeBlockFormat)

    enum class TableFormat {
        Open,     // Col | Col + ----+----
        Boxed,    // ┌┬┐ ... └┴┘
        Minimal   // Col | Col
    };
    Q_ENUM(TableFormat)

    enum class HorizontalRuleFormat {
        Dashed,    // Custom string, e.g. "--------"
        FullWidth  // ────── across entire width
    };
    Q_ENUM(HorizontalRuleFormat)

    Q_PROPERTY(Heading1Format h1Format READ h1Format WRITE setH1Format NOTIFY h1FormatChanged)
    Q_PROPERTY(int h1BadgePadding READ h1BadgePadding WRITE setH1BadgePadding NOTIFY h1BadgePaddingChanged)
    Q_PROPERTY(HeadingFormat headingFormat READ headingFormat WRITE setHeadingFormat NOTIFY headingFormatChanged)
    Q_PROPERTY(bool headingBanners READ headingBanners WRITE setHeadingBanners NOTIFY headingBannersChanged)
    Q_PROPERTY(CodeBlockFormat codeBlockFormat READ codeBlockFormat WRITE setCodeBlockFormat NOTIFY codeBlockFormatChanged)
    Q_PROPERTY(int codeBlockIndent READ codeBlockIndent WRITE setCodeBlockIndent NOTIFY codeBlockIndentChanged)
    Q_PROPERTY(TableFormat tableFormat READ tableFormat WRITE setTableFormat NOTIFY tableFormatChanged)
    Q_PROPERTY(HorizontalRuleFormat horizontalRuleFormat READ horizontalRuleFormat WRITE setHorizontalRuleFormat NOTIFY horizontalRuleFormatChanged)
    Q_PROPERTY(QString horizontalRuleText READ horizontalRuleText WRITE setHorizontalRuleText NOTIFY horizontalRuleTextChanged)
    Q_PROPERTY(QString blockquotePrefix READ blockquotePrefix WRITE setBlockquotePrefix NOTIFY blockquotePrefixChanged)
    Q_PROPERTY(QString listBulletLevel1 READ listBulletLevel1 WRITE setListBulletLevel1 NOTIFY listBulletLevel1Changed)
    Q_PROPERTY(QString listBulletLevel2 READ listBulletLevel2 WRITE setListBulletLevel2 NOTIFY listBulletLevel2Changed)
    Q_PROPERTY(QString taskListChecked READ taskListChecked WRITE setTaskListChecked NOTIFY taskListCheckedChanged)
    Q_PROPERTY(QString taskListUnchecked READ taskListUnchecked WRITE setTaskListUnchecked NOTIFY taskListUncheckedChanged)

    explicit MdDocStyle(QObject *parent = nullptr);
    ~MdDocStyle() override = default;

    Heading1Format h1Format() const { return m_h1Format; }
    void setH1Format(Heading1Format format);

    int h1BadgePadding() const { return m_h1BadgePadding; }
    void setH1BadgePadding(int padding);

    HeadingFormat headingFormat() const { return m_headingFormat; }
    void setHeadingFormat(HeadingFormat format);

    bool headingBanners() const { return m_headingBanners; }
    void setHeadingBanners(bool banners);

    CodeBlockFormat codeBlockFormat() const { return m_codeBlockFormat; }
    void setCodeBlockFormat(CodeBlockFormat format);

    int codeBlockIndent() const { return m_codeBlockIndent; }
    void setCodeBlockIndent(int indent);

    TableFormat tableFormat() const { return m_tableFormat; }
    void setTableFormat(TableFormat format);

    HorizontalRuleFormat horizontalRuleFormat() const { return m_horizontalRuleFormat; }
    void setHorizontalRuleFormat(HorizontalRuleFormat format);

    QString horizontalRuleText() const { return m_horizontalRuleText; }
    void setHorizontalRuleText(const QString &text);

    QString blockquotePrefix() const { return m_blockquotePrefix; }
    void setBlockquotePrefix(const QString &prefix);

    QString listBulletLevel1() const { return m_listBulletLevel1; }
    void setListBulletLevel1(const QString &bullet);

    QString listBulletLevel2() const { return m_listBulletLevel2; }
    void setListBulletLevel2(const QString &bullet);

    QString taskListChecked() const { return m_taskListChecked; }
    void setTaskListChecked(const QString &marker);

    QString taskListUnchecked() const { return m_taskListUnchecked; }
    void setTaskListUnchecked(const QString &marker);

signals:
    void h1FormatChanged();
    void h1BadgePaddingChanged();
    void headingFormatChanged();
    void headingBannersChanged();
    void codeBlockFormatChanged();
    void codeBlockIndentChanged();
    void tableFormatChanged();
    void horizontalRuleFormatChanged();
    void horizontalRuleTextChanged();
    void blockquotePrefixChanged();
    void listBulletLevel1Changed();
    void listBulletLevel2Changed();
    void taskListCheckedChanged();
    void taskListUncheckedChanged();
    void styleChanged();

private:
    Heading1Format m_h1Format = Heading1Format::Badge;
    int m_h1BadgePadding = 1;
    HeadingFormat m_headingFormat = HeadingFormat::Hashes;
    bool m_headingBanners = false;
    CodeBlockFormat m_codeBlockFormat = CodeBlockFormat::Indented;
    int m_codeBlockIndent = 2;
    TableFormat m_tableFormat = TableFormat::Open;
    HorizontalRuleFormat m_horizontalRuleFormat = HorizontalRuleFormat::Dashed;
    QString m_horizontalRuleText = QStringLiteral("--------");
    QString m_blockquotePrefix = QStringLiteral("│ ");
    QString m_listBulletLevel1 = QStringLiteral("• ");
    QString m_listBulletLevel2 = QStringLiteral("▪ ");
    QString m_taskListChecked = QStringLiteral("[✓] ");
    QString m_taskListUnchecked = QStringLiteral("[ ] ");
};

} // namespace qamui
