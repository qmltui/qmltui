#pragma once

#include "TextArea.h"
#include <QStringList>

namespace qtui {

class LogView : public TextArea {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int maximumLineCount READ maximumLineCount WRITE setMaximumLineCount NOTIFY maximumLineCountChanged)
    Q_PROPERTY(bool followTail READ followTail WRITE setFollowTail NOTIFY followTailChanged)

public:
    explicit LogView(QObject *parent = nullptr);

    int maximumLineCount() const { return m_maximumLineCount; }
    void setMaximumLineCount(int count);
    bool followTail() const { return m_followTail; }
    void setFollowTail(bool follow);

    Q_INVOKABLE void appendLine(const QString &line);
    Q_INVOKABLE void appendText(const QString &text);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void copySelection() { copy(); }

    void keyPressEvent(class KeyEvent *event) override;

signals:
    void maximumLineCountChanged();
    void followTailChanged();

private:
    void trimHistory();
    void scrollToTail();
    void applyBufferedText();

    int m_maximumLineCount = 10000;
    bool m_followTail = true;
    bool m_applyingBufferedText = false;
    QStringList m_lines;
};

} // namespace qtui
