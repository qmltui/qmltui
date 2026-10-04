#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "MdPalette.h"

namespace qtui {

class MdViewStyle : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qtui::MdPalette palette READ palette WRITE setPalette NOTIFY paletteChanged)
    Q_PROPERTY(bool showLineNumbers READ showLineNumbers WRITE setShowLineNumbers NOTIFY showLineNumbersChanged)
    Q_PROPERTY(QString borderStyle READ borderStyle WRITE setBorderStyle NOTIFY borderStyleChanged)

public:
    explicit MdViewStyle(QObject *parent = nullptr);
    ~MdViewStyle() override = default;

    MdPalette palette() const { return m_palette; }
    void setPalette(const MdPalette &palette);

    bool showLineNumbers() const { return m_showLineNumbers; }
    void setShowLineNumbers(bool show);

    QString borderStyle() const { return m_borderStyle; }
    void setBorderStyle(const QString &style);

signals:
    void paletteChanged();
    void showLineNumbersChanged();
    void borderStyleChanged();
    void styleChanged();

private:
    MdPalette m_palette;
    bool m_showLineNumbers = false;
    QString m_borderStyle = QStringLiteral("none");
};

} // namespace qtui
