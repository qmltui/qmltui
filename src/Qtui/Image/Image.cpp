#include <Qtui/Image/Image.h>
#include <Qtui/Image/ImageProvider.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <Qtui/Image/TerminalGraphics.h>
#include <Qtui/Core/primitives/ColorMapper.h>
#include <QDebug>
#include <QDirIterator>
#include <algorithm>
#include <QBuffer>
#include <QCache>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <cmath>

namespace qtui {

static const char* ASCII_RAMP = " .:-=+*#%@";
static const int ASCII_RAMP_LEN = 10;

// The Image transparency rule (docs/components/image.md, "Transparency").
// Returns the colour a source pixel shows once drawn into a cell, or 0 for
// "draws nothing". Zero alpha leaves the cell untouched. Partial alpha is
// composited source-over onto the background the cell already holds and the
// result is stored opaque, because a cell has no alpha channel. `behind` is 0
// when that background is the terminal default, whose real colour is unknown:
// then only pixels at least half opaque are drawn, unblended, the same cut the
// sixel encoder applies. QRgb and cell colours share the 0xAARRGGBB layout.
static uint32_t compositeOver(QRgb pixel, uint32_t behind)
{
    const int alpha = qAlpha(pixel);
    if (alpha == 0) return 0;
    if (alpha == 255) return pixel;
    if (behind == 0) return alpha >= 128 ? (0xFF000000u | (pixel & 0x00FFFFFFu)) : 0;
    const int inverse = 255 - alpha;
    const auto mix = [&](int source, int destination) {
        return (source * alpha + destination * inverse + 127) / 255;
    };
    return qRgb(mix(qRed(pixel), qRed(behind)),
                mix(qGreen(pixel), qGreen(behind)),
                mix(qBlue(pixel), qBlue(behind)));
}

// RGB of the background a cell holds, or 0 for the terminal default. ANSI
// palette entries resolve to xterm's default palette, the same approximation
// the encoder uses when it quantises.
static uint32_t cellBackgroundRgb(uint32_t bgColor)
{
    if ((bgColor >> 24) == 0) return 0;
    return 0xFF000000u | (ColorMapper::toArgb(bgColor) & 0x00FFFFFFu);
}

static constexpr uint8_t brailleBit(int dotX, int dotY) {
    constexpr uint8_t masks[2][4] = {
        { 0x01, 0x02, 0x04, 0x40 },
        { 0x08, 0x10, 0x20, 0x80 }
    };
    return masks[dotX & 1][dotY & 3];
}

// Sources fetched over the network rather than read from a file or resource.
static bool isRemote(const QUrl& url) {
    const QString scheme = url.scheme();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https") || scheme == QLatin1String("ftp");
}

// Decoded remote pictures by URL, so a grid of the same thumbnail, a relayout
// or a re-created item does not download again. Costed in bytes.
static QCache<QString, QImage>& remoteCache() {
    static QCache<QString, QImage> cache(32 * 1024 * 1024);
    return cache;
}

// The engine's manager when the item was created by QML (so cookies, proxies
// and a redirect policy set there apply), else one shared by every Image
// created from C++.
static QNetworkAccessManager* networkManager(QObject* item) {
    if (QQmlEngine* engine = qmlEngine(item))
        return engine->networkAccessManager();
    static QNetworkAccessManager* shared = new QNetworkAccessManager(QCoreApplication::instance());
    return shared;
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

Image::Image(QObject* parent) : Item(parent) {
    connect(this, &Item::widthChanged, this, &Image::renderSizeChanged);
    connect(this, &Item::heightChanged, this, &Image::renderSizeChanged);
}

graphics::Protocol Image::activeProtocol() const {
    if (m_renderMode != RenderMode::Auto && m_renderMode != RenderMode::Kitty && m_renderMode != RenderMode::Sixel)
        return graphics::Protocol::None;
    const TerminalCapabilities caps = terminal() ? terminal()->capabilities() : TerminalCapabilities{};
    const graphics::Protocol available = graphics::resolve(caps);
    // Auto takes the best available; an explicit mode is honoured only when
    // that protocol is the one available, else all fall back to HalfBlock.
    if (m_renderMode == RenderMode::Auto) return available;
    if (m_renderMode == RenderMode::Kitty && available == graphics::Protocol::Kitty) return available;
    if (m_renderMode == RenderMode::Sixel && available == graphics::Protocol::Sixel) return available;
    return graphics::Protocol::None;
}

void Image::watchCapabilities() const {
    Terminal* root = terminal();
    if (root == m_watchedTerminal) return;
    if (m_capabilitiesConnection) disconnect(m_capabilitiesConnection);
    m_watchedTerminal = root;
    if (root)
        m_capabilitiesConnection = connect(root, &Terminal::capabilitiesChanged, this, &Image::renderSizeChanged);
}

QSize Image::renderSize() const {
    watchCapabilities();
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) return QSize();
    const graphics::Protocol protocol = activeProtocol();
    if (protocol != graphics::Protocol::None) {
        // A frame beyond 4096 px a side is never useful in a terminal; clamp
        // uniformly so a producer's aspect ratio survives an odd cell size.
        const QSize cell = terminal()->capabilities().cellPixelSize;
        if (cell.isValid() && !cell.isEmpty()) {
            QSize pixels(w * cell.width(), h * cell.height());
            constexpr int maxEdge = 4096;
            if (pixels.width() > maxEdge || pixels.height() > maxEdge)
                pixels = pixels.scaled(maxEdge, maxEdge, Qt::KeepAspectRatio);
            return pixels;
        }
    }
    switch (m_renderMode) {
    case RenderMode::Braille: return QSize(w * 2, h * 4);
    case RenderMode::ASCII: return QSize(w, h);
    default: return QSize(w, h * 2);   // HalfBlock and every fallback
    }
}

Image::~Image() {
    cancelPendingResponse();
    cancelPendingReply();
    releaseKittyImage();
}

void Image::cancelPendingReply() {
    if (!m_reply) return;
    QNetworkReply* reply = m_reply;
    m_reply.clear();
    disconnect(reply, nullptr, this, nullptr);   // abort() emits finished() synchronously
    reply->abort();
    reply->deleteLater();
}

void Image::setProgress(qreal progress) {
    progress = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_progress + 1.0, progress + 1.0)) return;
    m_progress = progress;
    emit progressChanged();
}

void Image::loadFromNetwork(const QUrl& url) {
    setProgress(0.0);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Qtui.Image"));
    QNetworkReply* reply = networkManager(this)->get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 received, qint64 total) {
        if (reply != m_reply) return;
        if (total > 0) setProgress(static_cast<qreal>(received) / static_cast<qreal>(total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
        if (reply != m_reply) {   // superseded meanwhile
            reply->deleteLater();
            return;
        }
        m_reply.clear();
        reply->deleteLater();
        QImage image;
        if (reply->error() == QNetworkReply::NoError) {
            QBuffer buffer;
            buffer.setData(reply->readAll());
            buffer.open(QIODevice::ReadOnly);
            QImageReader reader(&buffer);
            image = reader.read();
            if (image.isNull()) {
                qWarning("Image: %s: %s", qPrintable(url.toString()), qPrintable(reader.errorString()));
            } else {
                image = image.convertToFormat(QImage::Format_ARGB32);
                remoteCache().insert(url.toString(), new QImage(image), static_cast<qsizetype>(image.sizeInBytes()));
            }
        } else {
            qWarning("Image: %s: %s", qPrintable(url.toString()), qPrintable(reply->errorString()));
        }
        releaseKittyImage();
        adoptImage(image, Status::Error);
    });
}

void Image::cancelPendingResponse() {
    if (!m_pendingResponse) return;
    ImageResponse* response = m_pendingResponse;
    m_pendingResponse.clear();
    disconnect(response, nullptr, this, nullptr);
    response->cancel();
    response->deleteLater();
}

void Image::releaseKittyImage() const {
    if (m_imageId == 0) return;
    if (Terminal* root = terminal())
        root->enqueueControlSequence(graphics::kittyDelete(m_imageId));
    m_imageId = 0;
    m_lastKittyKey = 0;
    m_lastKittyCells = QSize();
}

void Image::setSource(const QUrl& source) {
    if (m_source == source) return;
    releaseKittyImage();   // the old upload no longer describes this item
    m_source = source;
    emit sourceChanged();
    loadImage();
}

void Image::setImage(const QImage& image) {
    cancelPendingResponse();   // a picture assigned directly wins over one in flight
    cancelPendingReply();
    releaseKittyImage();   // a new picture is a new upload
    if (!m_source.isEmpty()) {
        // The picture no longer comes from a URL.
        m_source = QUrl();
        emit sourceChanged();
    }
    adoptImage(image.isNull() ? QImage() : image.convertToFormat(QImage::Format_ARGB32), Status::Null);
}

void Image::adoptImage(const QImage& image, Status statusIfNull) {
    const QSize previousSize = m_sourceImage.size();
    m_sourceImage = image;
    m_scaledImage = QImage();
    m_lastTargetSize = QSize();
    m_status = image.isNull() ? statusIfNull : Status::Ready;
    setProgress(image.isNull() ? 0.0 : 1.0);
    emit statusChanged();
    emit imageChanged();
    if (m_sourceImage.size() != previousSize) {
        emit sourceSizeChanged();
        emit implicitWidthChanged();
        emit implicitHeightChanged();
    }
    markDirty();
}

QImage Image::loadFromProvider(const QUrl& url, bool* found, bool* pending) {
    *found = false;
    *pending = false;
    QQmlEngine* engine = qmlEngine(this);
    if (!engine) return QImage();
    ImageProvider* provider = ImageProvider::find(engine, url.host());
    if (!provider) return QImage();
    *found = true;
    // Qt Quick's id: everything after "image://host/", query and fragment included.
    QString id = url.path();
    if (id.startsWith(QLatin1Char('/'))) id.remove(0, 1);
    if (url.hasQuery()) id += QLatin1Char('?') + url.query();
    if (url.hasFragment()) id += QLatin1Char('#') + url.fragment();
    // The pixel grid this item paints into, so a provider can render sharp.
    if (auto* async = qobject_cast<AsyncImageProvider*>(provider)) {
        ImageResponse* response = async->requestImageResponse(id, renderSize());
        if (!response) return QImage();          // refused: Error
        *pending = true;
        m_pendingResponse = response;
        // The current picture stays on screen until the new one arrives.
        connect(response, &ImageResponse::finished, this, [this, response] {
            if (response != m_pendingResponse) return;   // superseded meanwhile
            m_pendingResponse.clear();
            const QImage image = response->image();
            if (image.isNull() && !response->errorString().isEmpty())
                qWarning("Image: %s", qPrintable(response->errorString()));
            response->deleteLater();
            releaseKittyImage();
            adoptImage(image.isNull() ? QImage() : image.convertToFormat(QImage::Format_ARGB32), Status::Error);
        });
        return QImage();
    }
    QSize natural;
    return provider->requestImage(id, &natural, renderSize());
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
    emit renderSizeChanged();
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
        cancelPendingResponse();
        cancelPendingReply();
        setProgress(0.0);
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

    cancelPendingResponse();
    cancelPendingReply();
    m_status = Status::Loading;
    emit statusChanged();

    QImage loaded;
    if (isRemote(m_source)) {
        if (const QImage* cached = remoteCache().object(m_source.toString())) {
            loaded = *cached;
        } else {
            loadFromNetwork(m_source);
            return;   // Loading until the reply finishes; the old picture stays
        }
    } else if (m_source.scheme() == QLatin1String("image")) {
        bool found = false;
        bool pending = false;
        loaded = loadFromProvider(m_source, &found, &pending);
        if (pending)
            return;   // Loading until the response finishes; the old picture stays
        if (!found)
            qWarning("Image: no qtui::ImageProvider named \"%s\" on this engine",
                     qPrintable(m_source.host()));
    } else {
        loaded.load(resolvePath(m_source));
    }
    adoptImage(loaded.isNull() ? QImage() : loaded.convertToFormat(QImage::Format_ARGB32), Status::Error);
}

void Image::updateScaledCache(RenderMode mode) const {
    if (m_sourceImage.isNull()) return;

    int itemW = width();
    int itemH = height();
    if (itemW <= 0 || itemH <= 0) return;

    int targetSubW = itemW;
    int targetSubH = itemH;

    if (mode == RenderMode::HalfBlock) {
        targetSubW = itemW;
        targetSubH = itemH * 2;
    } else if (mode == RenderMode::Braille) {
        targetSubW = itemW * 2;
        targetSubH = itemH * 4;
    } else if (mode == RenderMode::ASCII) {
        targetSubW = itemW;
        targetSubH = itemH;
    }

    QSize currentTargetSize(targetSubW, targetSubH);
    if (m_lastTargetSize == currentTargetSize && m_lastFillMode == m_fillMode &&
        m_lastSmooth == m_smooth && m_lastRenderMode == mode && !m_scaledImage.isNull()) {
        return;
    }

    m_lastTargetSize = currentTargetSize;
    m_lastRenderMode = mode;
    m_lastFillMode = m_fillMode;
    m_lastSmooth = m_smooth;

    Qt::TransformationMode transMode = m_smooth ? Qt::SmoothTransformation : Qt::FastTransformation;

    if (mode == RenderMode::ASCII) {
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

void Image::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible() || m_status != Status::Ready || m_sourceImage.isNull())
        return;

    int itemW = width();
    int itemH = height();
    if (itemW <= 0 || itemH <= 0) return;

    // Resolve the effective mode. Auto picks the best pixel protocol the
    // terminal supports; an explicit Kitty falls back when unsupported. Both
    // collapse to HalfBlock so tests and plain terminals are unaffected.
    watchCapabilities();
    RenderMode mode = m_renderMode;
    if (mode == RenderMode::Auto || mode == RenderMode::Kitty || mode == RenderMode::Sixel) {
        const graphics::Protocol use = activeProtocol();
        if (use == graphics::Protocol::Kitty) {
            paintKitty(buffer, absX, absY, clipRect);
            return;
        }
        if (use == graphics::Protocol::Sixel) {
            paintSixel(buffer, absX, absY, clipRect);
            return;
        }
        mode = RenderMode::HalfBlock;
    }

    updateScaledCache(mode);
    if (m_scaledImage.isNull()) return;

    int subW = m_scaledImage.width();
    int subH = m_scaledImage.height();

    if (mode == RenderMode::HalfBlock) {
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

                const QRgb top = (px >= 0 && px < subW && topPy >= 0 && topPy < subH)
                                     ? m_scaledImage.pixel(px, topPy) : 0u;
                const QRgb bot = (px >= 0 && px < subW && botPy >= 0 && botPy < subH)
                                     ? m_scaledImage.pixel(px, botPy) : 0u;

                Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                const uint32_t behind = cellBackgroundRgb(cell.bgColor);
                const uint32_t topShown = compositeOver(top, behind);
                const uint32_t botShown = compositeOver(bot, behind);
                if (topShown == 0 && botShown == 0)
                    continue;   // both halves clear: the cell keeps its glyph and colours

                cell.displayWidth = 1;
                cell.continuation = false;
                // The glyph covers only the painted half, so a clear half shows
                // the background behind the image rather than a stale foreground.
                if (topShown == 0) {
                    cell.setCharacter("\xE2\x96\x84"); // ▄ Lower Half Block
                    cell.fgColor = botShown;
                } else {
                    cell.setCharacter("\xE2\x96\x80"); // ▀ Upper Half Block
                    cell.fgColor = topShown;
                    if (botShown != 0)
                        cell.bgColor = botShown;
                }
            }
        }
    } else if (mode == RenderMode::ASCII) {
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

                Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell)
                    continue;
                Cell& cell = *targetCell;
                // Glyph and colour both follow the composited pixel, so a faint
                // pixel over a dark background reads as a faint glyph.
                const uint32_t shown = compositeOver(m_scaledImage.pixel(px, py),
                                                     cellBackgroundRgb(cell.bgColor));
                if (shown == 0)
                    continue;

                const int brightness = qGray(shown);
                const int rampIndex = (brightness * (ASCII_RAMP_LEN - 1)) / 255;
                cell.displayWidth = 1;
                cell.continuation = false;
                const char asciiChar[2] = { ASCII_RAMP[rampIndex], '\0' };
                cell.setCharacter(asciiChar);
                cell.fgColor = shown;
            }
        }
    } else if (mode == RenderMode::Braille) {
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

                Cell *targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell)
                    continue;
                const uint32_t behind = cellBackgroundRgb(targetCell->bgColor);

                uint8_t cellValue = 0;
                uint32_t lastRgb = 0xFFFFFFFF;

                for (int dotY = 0; dotY < 4; ++dotY) {
                    for (int dotX = 0; dotX < 2; ++dotX) {
                        int px = cx * 2 + dotX - offsetX;
                        int py = cy * 4 + dotY - offsetY;
                        if (px < 0 || px >= subW || py < 0 || py >= subH)
                            continue;
                        // A dot is set when the composited pixel clears the dark
                        // cut-off, so a faint pixel over a dark background stays off.
                        const uint32_t shown = compositeOver(m_scaledImage.pixel(px, py), behind);
                        if (shown != 0 && qGray(shown) > 50) {
                            cellValue |= brailleBit(dotX, dotY);
                            lastRgb = shown;
                        }
                    }
                }

                if (cellValue > 0) {
                    Cell& cell = *targetCell;
                    cell.displayWidth = 1;
                    cell.continuation = false;
                    int brailleChar = 0x2800 + cellValue;
                    QChar qc(brailleChar);
                    cell.setCharacter(QString(qc).toUtf8());
                    cell.fgColor = lastRgb;
                }
            }
        }
    }
}

void Image::paintKitty(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    Terminal* root = terminal();
    if (!root) return;
    const int itemW = width();
    const int itemH = height();
    if (itemW <= 0 || itemH <= 0 || m_sourceImage.isNull()) return;

    const int sw = m_sourceImage.width();
    const int sh = m_sourceImage.height();
    if (sw <= 0 || sh <= 0) return;

    // Placement rectangle in cells. Stretch fills the item; PreserveAspectFit
    // shrinks to the source aspect and centres. Other modes behave as Stretch.
    int cols = itemW;
    int rows = itemH;
    int originX = absX;
    int originY = absY;
    if (m_fillMode == FillMode::PreserveAspectFit) {
        const QSize cell = root->capabilities().cellPixelSize;
        const double cw = cell.isValid() ? cell.width() : 1.0;
        const double ch = cell.isValid() ? cell.height() : 2.0;
        // Match pixel aspect: (cols*cw)/(rows*ch) == sw/sh, largest within item.
        int fitCols = itemW;
        int fitRows = static_cast<int>(std::lround(double(fitCols) * cw * sh / (double(sw) * ch)));
        if (fitRows > itemH) {
            fitRows = itemH;
            fitCols = static_cast<int>(std::lround(double(fitRows) * ch * sw / (double(sh) * cw)));
        }
        cols = std::clamp(fitCols, 1, itemW);
        rows = std::clamp(fitRows, 1, itemH);
        if (m_alignment & Qt::AlignHCenter) originX += (itemW - cols) / 2;
        else if (m_alignment & Qt::AlignRight) originX += itemW - cols;
        if (m_alignment & Qt::AlignVCenter) originY += (itemH - rows) / 2;
        else if (m_alignment & Qt::AlignBottom) originY += itemH - rows;
    }

    // Re-upload only when the source, the placement geometry or the upload
    // size changed. The terminal scales the stored image into the cols x rows
    // placement; a small source is pre-scaled here when `smooth` is set so the
    // magnification is filtered rather than blocky (see kittyUploadSize).
    const qint64 key = m_sourceImage.cacheKey();
    const QSize cellPx = root->capabilities().cellPixelSize;
    const QSize placementPx = cellPx.isValid() ? QSize(cols * cellPx.width(), rows * cellPx.height()) : QSize();
    const QSize uploadSize = graphics::kittyUploadSize(m_sourceImage.size(), placementPx, m_smooth);
    if (m_imageId == 0 || key != m_lastKittyKey || QSize(cols, rows) != m_lastKittyCells
        || uploadSize != m_lastKittyUpload) {
        if (m_imageId != 0)
            root->enqueueControlSequence(graphics::kittyDelete(m_imageId));
        QImage upload = m_sourceImage;
        if (uploadSize != m_sourceImage.size())
            upload = upload.scaled(uploadSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        m_lastKittyUpload = uploadSize;
        m_imageId = graphics::nextImageId();
        root->enqueueControlSequence(graphics::kittyTransmit(m_imageId, upload));
        root->enqueueControlSequence(graphics::kittyPlacement(m_imageId, cols, rows));
        m_lastKittyKey = key;
        m_lastKittyCells = QSize(cols, rows);
    }

    graphics::writeKittyPlaceholders(buffer, originX, originY, cols, rows, m_imageId, clipRect);
}

void Image::paintSixel(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    Terminal* root = terminal();
    if (!root) return;
    const int itemW = width();
    const int itemH = height();
    if (itemW <= 0 || itemH <= 0 || m_sourceImage.isNull()) return;
    const int sw = m_sourceImage.width();
    const int sh = m_sourceImage.height();
    if (sw <= 0 || sh <= 0) return;

    // Sixel needs real pixel dimensions. Use the reported cell size, or a
    // 10x20 (1:2) default that matches a typical monospace cell.
    const QSize cell = root->capabilities().cellPixelSize;
    const int cw = cell.isValid() ? cell.width() : 10;
    const int ch = cell.isValid() ? cell.height() : 20;

    // Placement rectangle in cells; PreserveAspectFit shrinks and centres.
    int cols = itemW;
    int rows = itemH;
    int originX = absX;
    int originY = absY;
    if (m_fillMode == FillMode::PreserveAspectFit) {
        int fitCols = itemW;
        int fitRows = static_cast<int>(std::lround(double(fitCols) * cw * sh / (double(sw) * ch)));
        if (fitRows > itemH) {
            fitRows = itemH;
            fitCols = static_cast<int>(std::lround(double(fitRows) * ch * sw / (double(sh) * cw)));
        }
        cols = std::clamp(fitCols, 1, itemW);
        rows = std::clamp(fitRows, 1, itemH);
        if (m_alignment & Qt::AlignHCenter) originX += (itemW - cols) / 2;
        else if (m_alignment & Qt::AlignRight) originX += itemW - cols;
        if (m_alignment & Qt::AlignVCenter) originY += (itemH - rows) / 2;
        else if (m_alignment & Qt::AlignBottom) originY += itemH - rows;
    }

    // Reserve the cells so the diff clears whatever was there and nothing else
    // paints into the image area. The sixel is drawn on top, after the diff.
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int x = originX + c;
            const int y = originY + r;
            if (!clipRect.contains(x, y)) continue;
            Cell* target = buffer.tryCellAt(x, y);
            if (!target) continue;
            target->setCharacter(" ");
            target->displayWidth = 1;
            target->continuation = false;
            target->fgColor = 0x00000000u;
            target->bgColor = 0x00000000u;
        }
    }

    // Re-emit only when the source, the geometry, or the output session
    // changed. A new session means the screen was cleared (resize, resume,
    // renderer switch), so the persisted sixel is gone and must be re-sent.
    const quint64 session = root->engine() ? root->engine()->outputSessionGeneration() : 0;
    const qint64 key = m_sourceImage.cacheKey();
    if (key == m_lastSixelKey && QSize(cols, rows) == m_lastSixelCells
        && session == m_lastSixelSession)
        return;
    m_lastSixelKey = key;
    m_lastSixelCells = QSize(cols, rows);
    m_lastSixelSession = session;

    QImage scaled = m_sourceImage.scaled(QSize(cols * cw, rows * ch),
                                         Qt::IgnoreAspectRatio,
                                         m_smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
    const QByteArray sixel = graphics::sixelEncode(scaled);
    if (sixel.isEmpty()) return;

    // Clear the whole item rectangle first. Sixel with P2=1 leaves unset
    // pixels untouched, so without an explicit clear the previous frame's
    // opaque pixels accumulate under the new one. Clearing the full item
    // (not just the placement) also erases a larger previous frame when a
    // mood change shrinks the placement. SGR is reset so blanks clear to the
    // default background; the next frame's diff re-establishes style.
    QByteArray positioned = "\033[0m\0337";            // default SGR, save cursor
    const QByteArray blank(itemW, ' ');
    for (int r = 0; r < itemH; ++r)
        positioned += "\033[" + QByteArray::number(absY + 1 + r) + ';'
                    + QByteArray::number(absX + 1) + 'H' + blank;
    positioned += "\033[" + QByteArray::number(originY + 1) + ';'
                + QByteArray::number(originX + 1) + 'H';    // move to the image origin
    positioned += sixel;
    positioned += "\0338";                               // restore cursor
    root->enqueuePostControlSequence(positioned);
}

} // namespace qtui
