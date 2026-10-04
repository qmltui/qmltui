#include "MdDocStyle.h"

namespace qtui {

MdDocStyle::MdDocStyle(QObject *parent)
    : QObject(parent) {
}

void MdDocStyle::setH1Format(Heading1Format format) {
    if (m_h1Format == format)
        return;
    m_h1Format = format;
    emit h1FormatChanged();
    emit styleChanged();
}

void MdDocStyle::setH1BadgePadding(int padding) {
    const int clamped = std::max(0, padding);
    if (m_h1BadgePadding == clamped)
        return;
    m_h1BadgePadding = clamped;
    emit h1BadgePaddingChanged();
    emit styleChanged();
}

void MdDocStyle::setHeadingFormat(HeadingFormat format) {
    if (m_headingFormat == format)
        return;
    m_headingFormat = format;
    emit headingFormatChanged();
    emit styleChanged();
}

void MdDocStyle::setHeadingBanners(bool banners) {
    if (m_headingBanners == banners)
        return;
    m_headingBanners = banners;
    emit headingBannersChanged();
    emit styleChanged();
}

void MdDocStyle::setCodeBlockFormat(CodeBlockFormat format) {
    if (m_codeBlockFormat == format)
        return;
    m_codeBlockFormat = format;
    emit codeBlockFormatChanged();
    emit styleChanged();
}

void MdDocStyle::setCodeBlockIndent(int indent) {
    const int clamped = std::max(0, indent);
    if (m_codeBlockIndent == clamped)
        return;
    m_codeBlockIndent = clamped;
    emit codeBlockIndentChanged();
    emit styleChanged();
}

void MdDocStyle::setTableFormat(TableFormat format) {
    if (m_tableFormat == format)
        return;
    m_tableFormat = format;
    emit tableFormatChanged();
    emit styleChanged();
}

void MdDocStyle::setHorizontalRuleFormat(HorizontalRuleFormat format) {
    if (m_horizontalRuleFormat == format)
        return;
    m_horizontalRuleFormat = format;
    emit horizontalRuleFormatChanged();
    emit styleChanged();
}

void MdDocStyle::setHorizontalRuleText(const QString &text) {
    if (m_horizontalRuleText == text)
        return;
    m_horizontalRuleText = text;
    emit horizontalRuleTextChanged();
    emit styleChanged();
}

void MdDocStyle::setBlockquotePrefix(const QString &prefix) {
    if (m_blockquotePrefix == prefix)
        return;
    m_blockquotePrefix = prefix;
    emit blockquotePrefixChanged();
    emit styleChanged();
}

void MdDocStyle::setListBulletLevel1(const QString &bullet) {
    if (m_listBulletLevel1 == bullet)
        return;
    m_listBulletLevel1 = bullet;
    emit listBulletLevel1Changed();
    emit styleChanged();
}

void MdDocStyle::setListBulletLevel2(const QString &bullet) {
    if (m_listBulletLevel2 == bullet)
        return;
    m_listBulletLevel2 = bullet;
    emit listBulletLevel2Changed();
    emit styleChanged();
}

void MdDocStyle::setTaskListChecked(const QString &marker) {
    if (m_taskListChecked == marker)
        return;
    m_taskListChecked = marker;
    emit taskListCheckedChanged();
    emit styleChanged();
}

void MdDocStyle::setTaskListUnchecked(const QString &marker) {
    if (m_taskListUnchecked == marker)
        return;
    m_taskListUnchecked = marker;
    emit taskListUncheckedChanged();
    emit styleChanged();
}

} // namespace qtui
