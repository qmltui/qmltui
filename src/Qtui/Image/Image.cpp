#include "Image.h"
#include <QDebug>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QPainter>
#include <cmath>

namespace qtui {

static const char* ASCII_RAMP = " .:-=+*#%@";
static const int ASCII_RAMP_LEN = 10;

static constexpr uint8_t brailleBit(int dotX, int dotY) {
    constexpr uint8_t masks[2][4] = {
        { 0x01, 0x02, 0x04, 0x40 },
        { 0x08, 0x10, 0x20, 0x80 }
    };
    return masks[dotX & 1][dotY & 3];
}

static QString resolvePath(const QUrl& url) {
    if (url.isEmpty()) return QString();

    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    QString str = url.toString();

    if (url.scheme() == QLatin1String("qrc")) {
        QString path = url.path();
        if (!path.startsWith(QLatin1Char('/'))) {
            path.prepend(QLatin1Char('/'));
        }
        QString qrcPath = QLatin1Char(':') + path;
        if (QFile::exists(qrcPath)) return qrcPath;
        str = path.startsWith(QLatin1Char('/')) ? path.mid(1) : path;
    }

    if (str.startsWith(QLatin1String("qrc:/"))) {
        QString qrcPath = QLatin1Char(':') + str.mid(4);
        if (QFile::exists(qrcPath)) return qrcPath;
        str = str.mid(5);
    }

    if (str.startsWith(QLatin1String(":"))) {
        if (QFile::exists(str)) return str;
    }

    if (str.startsWith(QLatin1String("file://"))) {
        return QUrl(str).toLocalFile();
    }

    if (QFile::exists(str)) {
        return str;
    }

    if (QFile::exists(QLatin1Char(':') + str)) {
        return QLatin1Char(':') + str;
    }

    if (QFile::exists(QStringLiteral(":/") + str)) {
        return QStringLiteral(":/") + str;
    }

    // Search in Qt QML module resource root (:/qt/qml/...)
    QDirIterator qmlIt(QStringLiteral(":/qt/qml"), QDirIterator::Subdirectories);
    while (qmlIt.hasNext()) {
        qmlIt.next();
        if (qmlIt.fileName() == str || qmlIt.filePath().endsWith(QLatin1Char('/') + str)) {
            return qmlIt.filePath();
        }
    }

    // Search in general QRC root (:/...)
    QDirIterator qrcIt(QStringLiteral(":"), QDirIterator::Subdirectories);
    while (qrcIt.hasNext()) {
        qrcIt.next();
        if (qrcIt.fileName() == str || qrcIt.filePath().endsWith(QLatin1Char('/') + str)) {
            return qrcIt.filePath();
        }
    }

    return str;
}

Image::Image(QObject* parent) : Item(parent) {}

void Image::setSource(const QUrl& source) {
    if (m_source == source) return;
    m_source = source;
    emit sourceChanged();
    loadImage();
}

void Image::setFillMode(FillMode mode) {
    if (m_fillMode == mode) return;
    m_fillMode = mode;
    emit fillModeChanged();
    markDirty();
}

void Image::setRenderMode(RenderMode mode) {
    if (m_renderMode == mode) return;
    m_renderMode = mode;
    emit renderModeChanged();
    markDirty();
}

void Image::setSmooth(bool smooth) {
    if (m_smooth == smooth) return;
    m_smooth = smooth;
    emit smoothChanged();
    markDirty();
}

void Image::setAlignment(Qt::Alignment alignment) {
    if (m_alignment == alignment) return;
    m_alignment = alignment;
    emit alignmentChanged();
    markDirty();
}

int Image::implicitWidth() const {
    if (m_sourceImage.isNull()) return m_implicitWidth;
    return m_sourceImage.width();
}

int Image::implicitHeight() const {
    if (m_sourceImage.isNull()) return m_implicitHeight;
    // For HalfBlock mode (default), 1 character cell is 2 vertical subpixels.
    return (m_sourceImage.height() + 1) / 2;
}

void Image::loadImage() {
    if (m_source.isEmpty()) {
        m_sourceImage = QImage();
        m_scaledImage = QImage();
        m_status = Status::Null;
        emit statusChanged();
        emit sourceSizeChanged();
        emit implicitWidthChanged();
        emit implicitHeightChanged();
        markDirty();
        return;
    }

    m_status = Status::Loading;
    emit statusChanged();

    QString path = resolvePath(m_source);

    QImage loaded;
    if (loaded.load(path)) {
        m_sourceImage = loaded.convertToFormat(QImage::Format_ARGB32);
        m_status = Status::Ready;
        m_lastTargetSize = QSize();
        emit implicitWidthChanged();
        emit implicitHeightChanged();
    } else {
        m_sourceImage = QImage();
        m_scaledImage = QImage();
        m_status = Status::Error;
    }

    emit statusChanged();
    emit sourceSizeChanged();
    markDirty();
}

void Image::updateScaledCache() {
    if (m_sourceImage.isNull()) return;

    int itemW = width();
    int itemH = height();
    if (itemW <= 0 || itemH <= 0) return;

    int targetSubW = itemW;
    int targetSubH = itemH;

    if (m_renderMode == RenderMode::HalfBlock) {
        targetSubW = itemW;
        targetSubH = itemH * 2;
    } else if (m_renderMode == RenderMode::Braille) {
        targetSubW = itemW * 2;
        targetSubH = itemH * 4;
    } else if (m_renderMode == RenderMode::ASCII) {
        targetSubW = itemW;
        targetSubH = itemH;
    }

    QSize currentTargetSize(targetSubW, targetSubH);
    if (m_lastTargetSize == currentTargetSize && m_lastFillMode == m_fillMode &&
        m_lastSmooth == m_smooth && !m_scaledImage.isNull()) {
        return;
    }

    m_lastTargetSize = currentTargetSize;
    m_lastFillMode = m_fillMode;
    m_lastSmooth = m_smooth;

    Qt::TransformationMode transMode = m_smooth ? Qt::SmoothTransformation : Qt::FastTransformation;

    if (m_renderMode == RenderMode::ASCII) {
        // Monospace cell aspect ratio correction (1 column : 2 rows visual ratio, cellAspect = 0.5)
        double srcAspect = static_cast<double>(m_sourceImage.width()) / std::max(1, m_sourceImage.height());
        
        if (m_fillMode == FillMode::Stretch) {
            m_scaledImage = m_sourceImage.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectFit) {
            // Visual aspect fit: cols / (rows * 2.0) = srcAspect  =>  cols = rows * 2.0 * srcAspect
            int fitW = targetSubW;
            int fitH = std::round(targetSubW / (srcAspect * 2.0));
            if (fitH > targetSubH) {
                fitH = targetSubH;
                fitW = std::round(targetSubH * srcAspect * 2.0);
            }
            fitW = std::clamp(fitW, 1, targetSubW);
            fitH = std::clamp(fitH, 1, targetSubH);
            m_scaledImage = m_sourceImage.scaled(QSize(fitW, fitH), Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectCrop) {
            int cropW = targetSubW;
            int cropH = std::round(targetSubW / (srcAspect * 2.0));
            if (cropH < targetSubH) {
                cropH = targetSubH;
                cropW = std::round(targetSubH * srcAspect * 2.0);
            }
            QImage scaledTemp = m_sourceImage.scaled(QSize(cropW, cropH), Qt::IgnoreAspectRatio, transMode);
            int startX = (scaledTemp.width() - targetSubW) / 2;
            int startY = (scaledTemp.height() - targetSubH) / 2;
            m_scaledImage = scaledTemp.copy(std::max(0, startX), std::max(0, startY), targetSubW, targetSubH);
        } else {
            m_scaledImage = m_sourceImage.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        }
    } else {
        if (m_fillMode == FillMode::Stretch) {
            m_scaledImage = m_sourceImage.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectFit) {
            m_scaledImage = m_sourceImage.scaled(currentTargetSize, Qt::KeepAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectCrop) {
            QImage expanded = m_sourceImage.scaled(currentTargetSize, Qt::KeepAspectRatioByExpanding, transMode);
            int cropX = (expanded.width() - targetSubW) / 2;
            int cropY = (expanded.height() - targetSubH) / 2;
            m_scaledImage = expanded.copy(cropX, cropY, targetSubW, targetSubH);
        } else if (m_fillMode == FillMode::Pad) {
            m_scaledImage = m_sourceImage;
        } else if (m_fillMode == FillMode::Tile) {
            m_scaledImage = QImage(currentTargetSize, QImage::Format_ARGB32);
            m_scaledImage.fill(Qt::transparent);
            QPainter p(&m_scaledImage);
            for (int y = 0; y < targetSubH; y += m_sourceImage.height()) {
                for (int x = 0; x < targetSubW; x += m_sourceImage.width()) {
                    p.drawImage(x, y, m_sourceImage);
                }
            }
        }
    }
}

void Image::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!visible() || m_status != Status::Ready || m_sourceImage.isNull())
        return;

    int itemW = width();
    int itemH = height();
    if (itemW <= 0 || itemH <= 0) return;

    updateScaledCache();
    if (m_scaledImage.isNull()) return;

    int subW = m_scaledImage.width();
    int subH = m_scaledImage.height();

    if (m_renderMode == RenderMode::HalfBlock) {
        // Compute alignment offsets in subpixel space
        int subTargetW = itemW;
        int subTargetH = itemH * 2;

        int offsetX = 0;
        int offsetY = 0;

        if (m_alignment & Qt::AlignHCenter) {
            offsetX = (subTargetW - subW) / 2;
        } else if (m_alignment & Qt::AlignRight) {
            offsetX = subTargetW - subW;
        }

        if (m_alignment & Qt::AlignVCenter) {
            offsetY = (subTargetH - subH) / 2;
        } else if (m_alignment & Qt::AlignBottom) {
            offsetY = subTargetH - subH;
        }

        for (int cy = 0; cy < itemH; ++cy) {
            for (int cx = 0; cx < itemW; ++cx) {
                int targetX = absX + cx;
                int targetY = absY + cy;

                if (!clipRect.contains(targetX, targetY))
                    continue;

                int topPy = cy * 2 - offsetY;
                int botPy = cy * 2 + 1 - offsetY;
                int px = cx - offsetX;

                QColor topColor = (px >= 0 && px < subW && topPy >= 0 && topPy < subH)
                                      ? m_scaledImage.pixelColor(px, topPy)
                                      : QColor(0, 0, 0, 0);

                QColor botColor = (px >= 0 && px < subW && botPy >= 0 && botPy < subH)
                                      ? m_scaledImage.pixelColor(px, botPy)
                                      : QColor(0, 0, 0, 0);

                if (topColor.alpha() < 10 && botColor.alpha() < 10)
                    continue;

                Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                cell.displayWidth = 1;
                cell.continuation = false;
                cell.character = "\xE2\x96\x80"; // ▀ Upper Half Block

                if (topColor.alpha() >= 10) {
                    cell.fgColor = topColor.rgba();
                }
                if (botColor.alpha() >= 10) {
                    cell.bgColor = botColor.rgba();
                }
            }
        }
    } else if (m_renderMode == RenderMode::ASCII) {
        int subTargetW = itemW;
        int subTargetH = itemH;

        int offsetX = 0;
        int offsetY = 0;

        if (m_alignment & Qt::AlignHCenter) {
            offsetX = (subTargetW - subW) / 2;
        } else if (m_alignment & Qt::AlignRight) {
            offsetX = subTargetW - subW;
        }

        if (m_alignment & Qt::AlignVCenter) {
            offsetY = (subTargetH - subH) / 2;
        } else if (m_alignment & Qt::AlignBottom) {
            offsetY = subTargetH - subH;
        }

        for (int cy = 0; cy < itemH; ++cy) {
            for (int cx = 0; cx < itemW; ++cx) {
                int targetX = absX + cx;
                int targetY = absY + cy;

                if (!clipRect.contains(targetX, targetY))
                    continue;

                int px = cx - offsetX;
                int py = cy - offsetY;

                if (px < 0 || px >= subW || py < 0 || py >= subH)
                    continue;

                QColor col = m_scaledImage.pixelColor(px, py);
                if (col.alpha() < 10)
                    continue;

                int brightness = qGray(col.rgb());
                int rampIndex = (brightness * (ASCII_RAMP_LEN - 1)) / 255;

                Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                cell.displayWidth = 1;
                cell.continuation = false;
                char asciiChar[2] = { ASCII_RAMP[rampIndex], '\0' };
                cell.character = asciiChar;
                cell.fgColor = col.rgba();
            }
        }
    } else if (m_renderMode == RenderMode::Braille) {
        // 2x4 subpixels per cell
        int subTargetW = itemW * 2;
        int subTargetH = itemH * 4;

        int offsetX = 0;
        int offsetY = 0;

        if (m_alignment & Qt::AlignHCenter) {
            offsetX = (subTargetW - subW) / 2;
        } else if (m_alignment & Qt::AlignRight) {
            offsetX = subTargetW - subW;
        }

        if (m_alignment & Qt::AlignVCenter) {
            offsetY = (subTargetH - subH) / 2;
        } else if (m_alignment & Qt::AlignBottom) {
            offsetY = subTargetH - subH;
        }

        for (int cy = 0; cy < itemH; ++cy) {
            for (int cx = 0; cx < itemW; ++cx) {
                int targetX = absX + cx;
                int targetY = absY + cy;

                if (!clipRect.contains(targetX, targetY))
                    continue;

                uint8_t cellValue = 0;
                uint32_t lastRgb = 0xFFFFFFFF;

                for (int dotY = 0; dotY < 4; ++dotY) {
                    for (int dotX = 0; dotX < 2; ++dotX) {
                        int px = cx * 2 + dotX - offsetX;
                        int py = cy * 4 + dotY - offsetY;
                        if (px >= 0 && px < subW && py >= 0 && py < subH) {
                            QColor col = m_scaledImage.pixelColor(px, py);
                            if (col.alpha() >= 10 && qGray(col.rgb()) > 50) {
                                cellValue |= brailleBit(dotX, dotY);
                                lastRgb = col.rgba();
                            }
                        }
                    }
                }

                if (cellValue > 0) {
                    Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                    if (!targetCell)
                        continue;
                    Cell& cell = *targetCell;
                    cell.displayWidth = 1;
                    cell.continuation = false;
                    int brailleChar = 0x2800 + cellValue;
                    QChar qc(brailleChar);
                    cell.character = QString(qc).toUtf8();
                    cell.fgColor = lastRgb;
                }
            }
        }
    }
}

} // namespace qtui
