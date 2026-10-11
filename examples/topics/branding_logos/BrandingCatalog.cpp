#include "BrandingCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QUrl>

BrandingCatalog::BrandingCatalog(QObject *parent)
    : QObject(parent)
{
    loadAssets();
}

QVariantMap BrandingCatalog::get(int index) const {
    if (index >= 0 && index < m_assets.size()) {
        return m_assets.at(index).toMap();
    }
    return QVariantMap();
}

QString BrandingCatalog::readFile(const QString &resourcePath, const QString &diskPath) const {
    if (QFile::exists(resourcePath)) {
        QFile file(resourcePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
    }
    if (QFile::exists(diskPath)) {
        QFile file(diskPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
    }
    const QString appRelative = QCoreApplication::applicationDirPath() + QLatin1Char('/') + diskPath;
    if (QFile::exists(appRelative)) {
        QFile file(appRelative);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString::fromUtf8(file.readAll());
        }
    }
    // Also check upward for docs/assets/branding
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        if (dir.exists(diskPath)) {
            QFile file(dir.filePath(diskPath));
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return QString::fromUtf8(file.readAll());
            }
        }
        if (!dir.cdUp()) break;
    }
    return QString();
}

QString BrandingCatalog::resolveAssetSource(const QString &fileName) const {
    const QString res = QStringLiteral(":/branding/") + fileName;
    if (QFile::exists(res)) {
        return QStringLiteral("qrc:/branding/") + fileName;
    }
    const QString rel = QStringLiteral("docs/assets/branding/") + fileName;
    if (QFile::exists(rel)) {
        return QUrl::fromLocalFile(QFileInfo(rel).absoluteFilePath()).toString();
    }
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 4; ++i) {
        if (dir.exists(rel)) {
            return QUrl::fromLocalFile(dir.filePath(rel)).toString();
        }
        if (!dir.cdUp()) break;
    }
    return QStringLiteral("qrc:/branding/") + fileName;
}

void BrandingCatalog::loadAssets() {
    m_assets.clear();

    // 1. Vector SVGs
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mark_dark");
        item[QStringLiteral("title")] = QStringLiteral("Mark (Dark)");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_mark.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("256 x 256 px");
        item[QStringLiteral("description")] = QStringLiteral("Primary mark for dark backgrounds: paper-white ring (#F6F7F9) and CRT phosphor green (#00FF66) prompt chevron.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("lockup_dark");
        item[QStringLiteral("title")] = QStringLiteral("Lockup Wordmark (Dark)");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_lockup_dark.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("474 x 170 px");
        item[QStringLiteral("description")] = QStringLiteral("Full Qamui wordmark: mark as 'Q', monoline letters, CRT phosphor block cursor on 'i'.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mark_tile");
        item[QStringLiteral("title")] = QStringLiteral("App Icon Tile");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_mark_tile.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("256 x 256 px");
        item[QStringLiteral("description")] = QStringLiteral("Mark centered on a rounded dark tile (#0B1020) for application icons and window titles.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mark_light");
        item[QStringLiteral("title")] = QStringLiteral("Mark (Light)");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_mark_light.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("256 x 256 px");
        item[QStringLiteral("description")] = QStringLiteral("Mark for light backgrounds: ink ring (#1A1D26) and high-contrast green (#00A651) prompt chevron.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("light");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("lockup_light");
        item[QStringLiteral("title")] = QStringLiteral("Lockup Wordmark (Light)");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_lockup_light.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("474 x 170 px");
        item[QStringLiteral("description")] = QStringLiteral("Full Qamui wordmark with ink glyphs and forest green accent for light terminal themes.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("light");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mark_mono");
        item[QStringLiteral("title")] = QStringLiteral("Mark Monochrome");
        item[QStringLiteral("category")] = QStringLiteral("Vector SVG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_mark_mono.svg"));
        item[QStringLiteral("dimensions")] = QStringLiteral("256 x 256 px");
        item[QStringLiteral("description")] = QStringLiteral("Single-color mark using currentColor: inherits terminal foreground across all colors.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }

    // 2. Pre-rendered ANSI & Braille cell glyphs
    const QString cellsContent = readFile(QStringLiteral(":/branding/qamui_mark_cells.txt"),
                                         QStringLiteral("docs/assets/branding/qamui_mark_cells.txt"));
    parseCells(cellsContent);

    // 3. Mascot ASCII Banner
    const QString mascotContent = readFile(QStringLiteral(":/branding/mascot_ascii.txt"),
                                          QStringLiteral("docs/assets/branding/mascot_ascii.txt"));
    parseMascot(mascotContent);

    // 4. Raster Sheet Previews
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("logo_sheet");
        item[QStringLiteral("title")] = QStringLiteral("Logo Family Sheet");
        item[QStringLiteral("category")] = QStringLiteral("Raster PNG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_logo_sheet.png"));
        item[QStringLiteral("dimensions")] = QStringLiteral("Multi-size sheet");
        item[QStringLiteral("description")] = QStringLiteral("Raster reference previewing assets at 200, 96, 48, 32, and 16px across palettes.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }
    {
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mark_cells_png");
        item[QStringLiteral("title")] = QStringLiteral("Mark Cells Sheet");
        item[QStringLiteral("category")] = QStringLiteral("Raster PNG");
        item[QStringLiteral("type")] = QStringLiteral("image");
        item[QStringLiteral("source")] = resolveAssetSource(QStringLiteral("qamui_mark_cells.png"));
        item[QStringLiteral("dimensions")] = QStringLiteral("Cell reference");
        item[QStringLiteral("description")] = QStringLiteral("Raster visual reference showing how half-block and Braille cell grids render.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }

    emit assetsChanged();
}

void BrandingCatalog::parseCells(const QString &content) {
    if (content.isEmpty()) return;
    const QStringList lines = content.split(QLatin1Char('\n'));
    QString currentSection;
    QString currentText;

    auto flushSection = [this, &currentSection, &currentText]() {
        if (currentSection.isEmpty() || currentText.trimmed().isEmpty()) return;
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("cells_") + currentSection.toLower().replace(QLatin1Char(' '), QLatin1Char('_'));
        item[QStringLiteral("type")] = QStringLiteral("ansi");
        item[QStringLiteral("source")] = QStringLiteral("qamui_mark_cells.txt");
        item[QStringLiteral("text")] = currentText;
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");

        if (currentSection.contains(QLatin1String("half blocks 24x12"))) {
            item[QStringLiteral("title")] = QStringLiteral("ANSI Half-Blocks 24x12");
            item[QStringLiteral("category")] = QStringLiteral("Terminal Cells");
            item[QStringLiteral("dimensions")] = QStringLiteral("24 cols x 12 rows");
            item[QStringLiteral("description")] = QStringLiteral("TrueColor 24-bit ANSI sequences with ▀ half-block subpixel glyphs (24x24 subpixels).");
        } else if (currentSection.contains(QLatin1String("half blocks 16x8"))) {
            item[QStringLiteral("title")] = QStringLiteral("ANSI Half-Blocks 16x8");
            item[QStringLiteral("category")] = QStringLiteral("Terminal Cells");
            item[QStringLiteral("dimensions")] = QStringLiteral("16 cols x 8 rows");
            item[QStringLiteral("description")] = QStringLiteral("Compact TrueColor half-block mark for CLI headers and status lines.");
        } else if (currentSection.contains(QLatin1String("half blocks 10x5"))) {
            item[QStringLiteral("title")] = QStringLiteral("ANSI Half-Blocks 10x5");
            item[QStringLiteral("category")] = QStringLiteral("Terminal Cells");
            item[QStringLiteral("dimensions")] = QStringLiteral("10 cols x 5 rows");
            item[QStringLiteral("description")] = QStringLiteral("Micro TrueColor half-block mark for small prompts and toolbars.");
        } else if (currentSection.contains(QLatin1String("braille 20x10"))) {
            item[QStringLiteral("title")] = QStringLiteral("Unicode Braille 20x10");
            item[QStringLiteral("category")] = QStringLiteral("Braille Glyphs");
            item[QStringLiteral("dimensions")] = QStringLiteral("20 cols x 10 rows");
            item[QStringLiteral("description")] = QStringLiteral("2x4 dot subpixel grid using Unicode Braille Patterns (U+2800..U+28FF) for text consoles.");
        } else if (currentSection.contains(QLatin1String("braille 12x6"))) {
            item[QStringLiteral("title")] = QStringLiteral("Unicode Braille 12x6");
            item[QStringLiteral("category")] = QStringLiteral("Braille Glyphs");
            item[QStringLiteral("dimensions")] = QStringLiteral("12 cols x 6 rows");
            item[QStringLiteral("description")] = QStringLiteral("Medium-resolution Braille dot-matrix representation.");
        } else if (currentSection.contains(QLatin1String("braille 8x4"))) {
            item[QStringLiteral("title")] = QStringLiteral("Unicode Braille 8x4");
            item[QStringLiteral("category")] = QStringLiteral("Braille Glyphs");
            item[QStringLiteral("dimensions")] = QStringLiteral("8 cols x 4 rows");
            item[QStringLiteral("description")] = QStringLiteral("Ultra-compact 8x4 Braille pattern badge.");
        } else {
            item[QStringLiteral("title")] = currentSection;
            item[QStringLiteral("category")] = QStringLiteral("Terminal Cells");
            item[QStringLiteral("dimensions")] = QString();
            item[QStringLiteral("description")] = QStringLiteral("Pre-rendered terminal cell representation.");
        }
        m_assets.append(item);
        currentText.clear();
    };

    for (const QString &line : lines) {
        if (line.startsWith(QLatin1String("--- ")) && line.endsWith(QLatin1String(" ---"))) {
            flushSection();
            currentSection = line.mid(4, line.length() - 8).trimmed();
        } else {
            if (!currentText.isEmpty()) currentText += QLatin1Char('\n');
            currentText += line;
        }
    }
    flushSection();
}

void BrandingCatalog::parseMascot(const QString &content) {
    if (content.isEmpty()) return;
    const int idx = content.indexOf(QLatin1String("# 5. CLI Startup Banner: Qamui"));
    if (idx != -1) {
        const int start = content.indexOf(QLatin1Char('\n'), idx) + 1;
        const QString banner = content.mid(start).trimmed();
        QVariantMap item;
        item[QStringLiteral("id")] = QStringLiteral("mascot_qamui_banner");
        item[QStringLiteral("title")] = QStringLiteral("Mascot Startup Banner");
        item[QStringLiteral("category")] = QStringLiteral("ASCII Art");
        item[QStringLiteral("type")] = QStringLiteral("ansi");
        item[QStringLiteral("source")] = QStringLiteral("mascot_ascii.txt");
        item[QStringLiteral("text")] = banner;
        item[QStringLiteral("dimensions")] = QStringLiteral("64 cols x 5 rows");
        item[QStringLiteral("description")] = QStringLiteral("The Qamui mark in braille cells, an O whose > tail makes the Q, beside the startup banner text.");
        item[QStringLiteral("recommendedTheme")] = QStringLiteral("dark");
        m_assets.append(item);
    }
}
