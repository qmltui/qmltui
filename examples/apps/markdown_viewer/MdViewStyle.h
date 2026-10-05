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

public:
    explicit MdViewStyle(QObject *parent = nullptr);
    ~MdViewStyle() override = default;

    MdPalette palette() const { return m_palette; }
    void setPalette(const MdPalette &palette);

    bool showLineNumbers() const { return m_showLineNumbers; }
    void setShowLineNumbers(bool show);

signals:
    void paletteChanged();
    void showLineNumbersChanged();
    void styleChanged();

private:
    MdPalette m_palette;
    bool m_showLineNumbers = false;
};

} // namespace qtui
