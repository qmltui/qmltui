#include "MdViewStyle.h"

namespace qtui {

MdViewStyle::MdViewStyle(QObject *parent)
    : QObject(parent) {
}

void MdViewStyle::setPalette(const MdPalette &palette) {
    if (m_palette == palette)
        return;
    m_palette = palette;
    emit paletteChanged();
    emit styleChanged();
}

void MdViewStyle::setShowLineNumbers(bool show) {
    if (m_showLineNumbers == show)
        return;
    m_showLineNumbers = show;
    emit showLineNumbersChanged();
    emit styleChanged();
}

void MdViewStyle::setBorderStyle(const QString &style) {
    const QString normalized = style.trimmed().toLower();
    const QString value = normalized.isEmpty() ? QStringLiteral("none") : normalized;
    if (m_borderStyle == value)
        return;
    m_borderStyle = value;
    emit borderStyleChanged();
    emit styleChanged();
}

} // namespace qtui
