#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>
#include <QmlTui/Core/Items/TextArea.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/HighlightRange.h>
#include <QtQmlIntegration/qqmlintegration.h>

namespace qmltui::demo {

class JsonHighlighter : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qmltui::TextArea* target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm NOTIFY searchTermChanged)
    Q_PROPERTY(qmltui::Color stringColor READ stringColor WRITE setStringColor NOTIFY stylesChanged)
    Q_PROPERTY(qmltui::Color numberColor READ numberColor WRITE setNumberColor NOTIFY stylesChanged)
    Q_PROPERTY(
        qmltui::Color keywordColor READ keywordColor WRITE setKeywordColor NOTIFY stylesChanged)
    Q_PROPERTY(
        qmltui::Color commentColor READ commentColor WRITE setCommentColor NOTIFY stylesChanged)
    Q_PROPERTY(
        qmltui::Color searchBgColor READ searchBgColor WRITE setSearchBgColor NOTIFY stylesChanged)

public:
    enum TokenType {
        String,
        Number,
        Keyword,
        Comment,
    };
    Q_ENUM(TokenType)

    struct Token {
        int start = 0;
        int length = 0;
        TokenType type = String;
    };

    explicit JsonHighlighter(QObject* parent = nullptr);
    ~JsonHighlighter() override;

    TextArea* target() const { return m_target; }
    void setTarget(TextArea* target);

    QString searchTerm() const { return m_searchTerm; }
    void setSearchTerm(const QString& term);

    Color stringColor() const { return m_stringColor; }
    void setStringColor(Color color);

    Color numberColor() const { return m_numberColor; }
    void setNumberColor(Color color);

    Color keywordColor() const { return m_keywordColor; }
    void setKeywordColor(Color color);

    Color commentColor() const { return m_commentColor; }
    void setCommentColor(Color color);

    Color searchBgColor() const { return m_searchBgColor; }
    void setSearchBgColor(Color color);

    Q_INVOKABLE void rehighlight();

signals:
    void targetChanged();
    void searchTermChanged();
    void stylesChanged();

private slots:
    void onTargetTextChanged();

private:
    void tokenize();
    void applySyntaxHighlighting();
    void applySearchHighlighting();
    void detachTarget();

    QPointer<TextArea> m_target;
    QString m_searchTerm;

    Color m_stringColor = Color("green");
    Color m_numberColor = Color("cyan");
    Color m_keywordColor = Color("blue");
    Color m_commentColor = Color("#888888");
    Color m_searchBgColor = Color("yellow");

    QVector<Token> m_cachedTokens;
};

} // namespace qmltui::demo
