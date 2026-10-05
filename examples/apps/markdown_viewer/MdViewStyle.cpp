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

} // namespace qtui
