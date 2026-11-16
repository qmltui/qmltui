#include <QmlTui/Image/Image.h>
#include <QmlTui/Image/ImageProvider.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Image/TerminalGraphics.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/Subpixel.h>
#include <QDebug>
#include <algorithm>
#include <QBuffer>
#include <QCache>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QQmlInfo>
#include <cmath>

namespace qmltui {

static const char* ASCII_RAMP = " .:-=+*#%@";
static const int ASCII_RAMP_LEN = 10;

// Qt's QSvgPlugin queries QFontDatabase when rendering SVGs with <text>, font
// attributes, or em/ex units, calling qFatal() when no QGuiApplication exists.
static bool isSvgWithoutGuiApp(QIODevice* device, const QByteArray& declaredFormat) {
    if (qobject_cast<QGuiApplication*>(QCoreApplication::instance()) != nullptr) return false;
    const QByteArray fmt = declaredFormat.toLower();
    if (fmt == "svg" || fmt == "svgz") return true;
    if (!device || !device->isOpen()) return false;
    const qint64 savedPos = device->pos();
    QImageReader contentProbe(device);
    contentProbe.setDecideFormatFromContent(true);
    const QByteArray contentFmt = contentProbe.format().toLower();
    device->seek(savedPos);
    return contentFmt == "svg" || contentFmt == "svgz";
}

// The Image transparency rule (docs/qml-types/image.md, "Transparency").
// Returns the colour a source pixel shows once drawn into a cell, or 0 for
// "draws nothing". Zero alpha leaves the cell untouched. Partial alpha is
// composited source-over onto the background the cell already holds and the
// result is stored opaque, because a cell has no alpha channel. `behind` is 0
// when that background is the terminal default, whose real colour is unknown:
// then only pixels at least half opaque are drawn, unblended, the same cut the
// sixel encoder applies. QRgb and cell colours share the 0xAARRGGBB layout.
static uint32_t compositeOver(QRgb pixel, uint32_t behind) {
    const int alpha = qAlpha(pixel);
    if (alpha == 0) return 0;
    if (alpha == 255) return pixel;
    if (behind == 0) return alpha >= 128 ? (0xFF000000u | (pixel & 0x00FFFFFFu)) : 0;
    const int inverse = 255 - alpha;
    const auto mix = [&](int source, int destination) {
        return (source * alpha + destination * inverse + 127) / 255;
    };
    return qRgb(mix(qRed(pixel), qRed(behind)), mix(qGreen(pixel), qGreen(behind)),
                mix(qBlue(pixel), qBlue(behind)));
}

// RGB of the background a cell holds, or 0 for the terminal default. ANSI
// palette entries resolve to xterm's default palette, the same approximation
// the encoder uses when it quantises.
static uint32_t cellBackgroundRgb(uint32_t bgColor) {
    if ((bgColor >> 24) == 0) return 0;
    return 0xFF000000u | (Color::toArgb(bgColor) & 0x00FFFFFFu);
}

// Sources fetched over the network rather than read from a file or resource.
static bool isRemote(const QUrl& url) {
    const QString scheme = url.scheme();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https") ||
           scheme == QLatin1String("ftp");
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
    if (QQmlEngine* engine = qmlEngine(item)) return engine->networkAccessManager();
    static QPointer<QNetworkAccessManager> shared;
    if (!shared) shared = new QNetworkAccessManager(QCoreApplication::instance());
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

    if (QFile::exists(QStringLiteral(":/qt/qml/") + str)) {
        return QStringLiteral(":/qt/qml/") + str;
    }

    return str;
}

Image::Image(QObject* parent) : Item(parent) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    connect(this, &Item::widthChanged, this, &Image::renderSizeChanged);
    connect(this, &Item::heightChanged, this, &Image::renderSizeChanged);
    connect(this, &Item::visibleChanged, this, [this] {
        if (!visible()) releaseSixelImage();
    });
}

graphics::Protocol Image::activeProtocol() const {
    if (m_renderMode != RenderMode::Auto && m_renderMode != RenderMode::Kitty &&
        m_renderMode != RenderMode::Sixel)
        return graphics::Protocol::None;
    const TerminalCapabilities caps =
        terminal() ? terminal()->capabilities() : TerminalCapabilities{};
    const graphics::Protocol available = graphics::resolve(caps);
    // Auto takes the best available; an explicit mode is honoured only when
    // that protocol is the one available, else all fall back to HalfBlock.
    if (m_renderMode == RenderMode::Auto) return available;
    if (m_renderMode == RenderMode::Kitty && available == graphics::Protocol::Kitty)
        return available;
    if (m_renderMode == RenderMode::Sixel && available == graphics::Protocol::Sixel)
        return available;
    return graphics::Protocol::None;
}

void Image::watchCapabilities() const {
    Terminal* root = terminal();
    if (root == m_watchedTerminal) return;
    if (m_capabilitiesConnection) disconnect(m_capabilitiesConnection);
    m_watchedTerminal = root;
    if (root)
        m_capabilitiesConnection =
            connect(root, &Terminal::capabilitiesChanged, this, &Image::renderSizeChanged);
}

QSize Image::renderSize() const {
    watchCapabilities();
    const int w = width();
    const int h = height();
    if (w <= 0 || h <= 0) return QSize();
    const graphics::Protocol protocol = activeProtocol();
    if (protocol != graphics::Protocol::None) {
        // Match the upload limit so live producers don't paint pixels that
        // will immediately be scaled away. Preserve the pixel aspect ratio.
        const QSize cell = terminal()->capabilities().cellPixelSize;
        if (cell.isValid() && !cell.isEmpty()) {
            QSize pixels(w * cell.width(), h * cell.height());
            if (protocol == graphics::Protocol::Kitty)
                return graphics::kittyUploadSize(pixels, pixels, false);
            constexpr int maxEdge = 4096;
            if (pixels.width() > maxEdge || pixels.height() > maxEdge)
                pixels = pixels.scaled(maxEdge, maxEdge, Qt::KeepAspectRatio);
            return pixels;
        }
    }
    switch (m_renderMode) {
    case RenderMode::Braille:
        return QSize(w * braille::DotsX, h * braille::DotsY);
    case RenderMode::ASCII:
        return QSize(w, h);
    case RenderMode::HalfBlock:
    case RenderMode::Kitty:
    case RenderMode::Sixel:
    case RenderMode::Auto:
        return QSize(w * half_block::SubpixelsX, h * half_block::SubpixelsY);
    }
    return QSize(w * half_block::SubpixelsX, h * half_block::SubpixelsY);
}

Image::~Image() {
    cancelPendingResponse();
    cancelPendingReply();
    releaseKittyImage();
    releaseSixelImage();
}

void Image::cancelPendingReply() {
    if (!m_reply) return;
    QNetworkReply* reply = m_reply;
    m_reply.clear();
    disconnect(reply, nullptr, this, nullptr); // abort() emits finished() synchronously
    reply->abort();
    reply->deleteLater();
}

void Image::setProgress(qreal progress) {
    progress = (std::clamp)(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_progress + 1.0, progress + 1.0)) return;
    m_progress = progress;
    emit progressChanged();
}

void Image::loadFromNetwork(const QUrl& url) {
    static constexpr qint64 MaxNetworkImageBytes = 32LL * 1024 * 1024;
    setProgress(0.0);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QmlTui.Image"));
    request.setTransferTimeout(15000);
    QNetworkReply* reply = networkManager(this)->get(request);
    m_reply = reply;

    auto abortOversized = [this, reply, url]() {
        if (reply != m_reply) return;
        m_reply.clear();
        disconnect(reply, nullptr, this, nullptr);
        reply->abort();
        reply->deleteLater();
        qmlWarning(this) << "Image: " << url.toString() << ": response exceeds 32 MiB limit";
        adoptImage(QImage(), Status::Error);
    };

    connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, abortOversized] {
        if (reply != m_reply) return;
        const QVariant lenHeader = reply->header(QNetworkRequest::ContentLengthHeader);
        if (lenHeader.isValid() && lenHeader.toLongLong() > MaxNetworkImageBytes) {
            abortOversized();
        }
    });
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, reply, abortOversized](qint64 received, qint64 total) {
                if (reply != m_reply) return;
                if (received > MaxNetworkImageBytes || total > MaxNetworkImageBytes ||
                    reply->bytesAvailable() > MaxNetworkImageBytes) {
                    abortOversized();
                    return;
                }
                if (total > 0)
                    setProgress(static_cast<qreal>(received) / static_cast<qreal>(total));
            });
    connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
        if (reply != m_reply) { // superseded meanwhile
            reply->deleteLater();
            return;
        }
        m_reply.clear();
        reply->deleteLater();
        QImage image;
        if (reply->error() == QNetworkReply::NoError) {
            if (reply->bytesAvailable() > MaxNetworkImageBytes) {
                qmlWarning(this) << "Image: " << url.toString()
                                 << ": response exceeds 32 MiB limit";
            } else {
                QBuffer buffer;
                buffer.setData(reply->readAll());
                buffer.open(QIODevice::ReadOnly);
                QImageReader reader(&buffer);
                if (isSvgWithoutGuiApp(&buffer, reader.format())) {
                    qmlWarning(this)
                        << "Image: " << url.toString() << ": SVG sources require QGuiApplication";
                } else {
                    image = reader.read();
                    if (image.isNull()) {
                        qmlWarning(this)
                            << "Image: " << url.toString() << ": " << reader.errorString();
                    } else {
                        image = image.convertToFormat(QImage::Format_ARGB32);
                        remoteCache().insert(url.toString(), new QImage(image),
                                             static_cast<qsizetype>(image.sizeInBytes()));
                    }
                }
            }
        } else {
            qmlWarning(this) << "Image: " << url.toString() << ": " << reply->errorString();
        }
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

void Image::releaseKittyImage() {
    m_kittyPayloadCache.clear();
    if (m_imageId == 0) return;
    if (Terminal* root = terminal()) root->enqueueControlSequence(graphics::kittyDelete(m_imageId));
    m_imageId = 0;
    m_lastKittyKey = 0;
    m_lastKittyCells = QSize();
}

void Image::releaseSixelImage() {
    if (m_sixelPaintConnection) disconnect(m_sixelPaintConnection);
    m_paintedSixelThisFrame = false;
    m_lastSixelKey = 0;
    m_lastSixelPixels = QSize();
    m_lastSixelViewport = QSize();
    m_lastSixelSession = 0;
    m_lastSixelFrame = 0;
    m_lastSixelSmooth = false;
    m_hasLastSixelFrame = false;
    Terminal* root = m_sixelTerminal ? m_sixelTerminal.data() : terminal();
    m_sixelTerminal.clear();
    if (m_lastSixelRect.isEmpty()) return;
    if (root) {
        // Sixels persist outside the cell buffer. Erase before the next cell
        // diff so underlying content can be painted back over these blanks.
        QByteArray clear = "\033[0m\0337";
        const QByteArray blank(m_lastSixelRect.width(), ' ');
        for (int y = m_lastSixelRect.top(); y <= m_lastSixelRect.bottom(); ++y) {
            const int rowCoord = (std::max)(1, y + 1);
            const int colCoord = (std::max)(1, m_lastSixelRect.left() + 1);
            clear += "\033[" + QByteArray::number(rowCoord) + ';' + QByteArray::number(colCoord) +
                     'H' + blank;
        }
        clear += "\0338";
        if (root->enqueueControlSequence(clear)) {
            if (TerminalEngine* engine = root->engine())
                engine->invalidateCommittedRect(m_lastSixelRect);
        }
    }
    m_lastSixelRect = QRect();
}

void Image::setSource(const QUrl& source) {
    if (m_source == source) return;
    m_source = source;
    emit sourceChanged();
    loadImage();
}

void Image::setImage(const QImage& image) {
    cancelPendingResponse(); // a picture assigned directly wins over one in flight
    cancelPendingReply();
    if (!m_source.isEmpty()) {
        // The picture no longer comes from a URL.
        m_source = QUrl();
        emit sourceChanged();
    }
    adoptImage(image.isNull() ? QImage() : image.convertToFormat(QImage::Format_ARGB32),
               Status::Null);
}

void Image::adoptImage(const QImage& image, Status statusIfNull) {
    // Keep a stable Kitty id across frames: replacing its pixels and virtual
    // placement leaves the placeholder cells unchanged. Clearing still frees it.
    if (image.isNull()) releaseKittyImage();
    if (image.isNull()) releaseSixelImage();
    const QSize previousSize = m_sourceImage.size();
    m_sourceImage = image;
    m_scaledImage = QImage();
    m_lastTargetSize = QSize();
    m_clippedImage = QImage();
    m_clippedSourceKey = 0;
    m_clippedForRect = QRect();
    m_clippedFrameCache.clear();
    m_kittyPayloadCache.clear();
    m_status = image.isNull() ? statusIfNull : Status::Ready;
    setProgress(image.isNull() ? 0.0 : 1.0);
    emit statusChanged();
    emit imageChanged();
    if (m_sourceImage.size() != previousSize) {
        emit sourceSizeChanged();
    }
    notifyImplicitSizeIfChanged();
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
        if (!response) return QImage(); // refused: Error
        *pending = true;
        m_pendingResponse = response;
        // With retainWhileLoading the current picture stays until this arrives.
        connect(response, &ImageResponse::finished, this, [this, response] {
            if (response != m_pendingResponse) return; // superseded meanwhile
            m_pendingResponse.clear();
            const QImage image = response->image();
            if (image.isNull() && !response->errorString().isEmpty())
                qmlWarning(this) << "Image: " << response->errorString();
            response->deleteLater();
            adoptImage(image.isNull() ? QImage() : image.convertToFormat(QImage::Format_ARGB32),
                       Status::Error);
        });
        return QImage();
    }
    QSize natural;
    return provider->requestImage(id, &natural, renderSize());
}

const QImage& Image::effectiveImage() const {
    if (m_sourceClipRect.isNull() || m_sourceImage.isNull()) return m_sourceImage;
    const QRect bounded = m_sourceClipRect.intersected(m_sourceImage.rect());
    const qint64 key = m_sourceImage.cacheKey();
    if (key != m_clippedSourceKey || bounded != m_clippedForRect) {
        if (bounded.isEmpty()) {
            m_clippedImage = QImage();
        } else {
            bool found = false;
            for (const ClippedFrameCacheEntry& entry : std::as_const(m_clippedFrameCache)) {
                if (entry.sourceKey == key && entry.rect == bounded) {
                    m_clippedImage = entry.image;
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_clippedImage = m_sourceImage.copy(bounded);
                constexpr qsizetype kMaxCachedFrames = 32;
                if (m_clippedFrameCache.size() >= kMaxCachedFrames)
                    m_clippedFrameCache.removeFirst();
                m_clippedFrameCache.append(ClippedFrameCacheEntry{key, bounded, m_clippedImage});
            }
        }
        m_clippedSourceKey = key;
        m_clippedForRect = bounded;
    }
    return m_clippedImage;
}

void Image::setSourceClipRect(const QRect& rect) {
    if (m_sourceClipRect == rect) return;
    m_sourceClipRect = rect;
    m_scaledImage = QImage();
    m_lastTargetSize = QSize();
    if (m_status != Status::Loading && effectiveImage().isNull()) {
        releaseKittyImage();
        releaseSixelImage();
    }
    emit sourceClipRectChanged();
    notifyImplicitSizeIfChanged();
    markDirty();
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
    if (activeProtocol() != graphics::Protocol::Sixel) releaseSixelImage();
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

void Image::setRetainWhileLoading(bool retain) {
    if (m_retainWhileLoading == retain) return;
    m_retainWhileLoading = retain;
    if (!retain && m_status == Status::Loading && !m_sourceImage.isNull())
        adoptImage(QImage(), Status::Loading);
    emit retainWhileLoadingChanged();
}

void Image::setAlignment(Qt::Alignment alignment) {
    if (m_alignment == alignment) return;
    m_alignment = alignment;
    emit alignmentChanged();
    markDirty();
}

int Image::implicitWidth() const {
    const QImage& source = effectiveImage();
    if (source.isNull()) return Item::implicitWidth();
    return source.width();
}

int Image::implicitHeight() const {
    const QImage& source = effectiveImage();
    if (source.isNull()) return Item::implicitHeight();
    // For HalfBlock mode (default), 1 character cell is 2 vertical subpixels.
    return (source.height() + 1) / 2;
}

void Image::loadImage() {
    if (m_source.isEmpty()) {
        cancelPendingResponse();
        cancelPendingReply();
        releaseKittyImage();
        releaseSixelImage();
        setProgress(0.0);
        m_sourceImage = QImage();
        m_scaledImage = QImage();
        m_clippedImage = QImage();
        m_clippedSourceKey = 0;
        m_clippedForRect = QRect();
        m_clippedFrameCache.clear();
        m_kittyPayloadCache.clear();
        m_status = Status::Null;
        emit statusChanged();
        emit imageChanged();
        emit sourceSizeChanged();
        notifyImplicitSizeIfChanged();
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
            if (!m_retainWhileLoading) adoptImage(QImage(), Status::Loading);
            loadFromNetwork(m_source);
            return; // Loading until the reply finishes
        }
    } else if (m_source.scheme() == QLatin1String("image")) {
        bool found = false;
        bool pending = false;
        loaded = loadFromProvider(m_source, &found, &pending);
        if (pending) {
            if (!m_retainWhileLoading) adoptImage(QImage(), Status::Loading);
            return; // Loading until the response finishes
        }
        if (!found)
            qmlWarning(this) << "Image: no qmltui::ImageProvider named \"" << m_source.host()
                             << "\" on this engine";
    } else {
        const QString resolvedPath = resolvePath(m_source);
        QFile file(resolvedPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qmlWarning(this) << "Image: failed to load \"" << m_source.toString() << "\"";
        } else {
            QImageReader reader(&file);
            if (isSvgWithoutGuiApp(&file, reader.format())) {
                qmlWarning(this) << "Image: \"" << m_source.toString()
                                 << "\": SVG sources require QGuiApplication";
            } else {
                loaded = reader.read();
                if (loaded.isNull())
                    qmlWarning(this) << "Image: failed to load \"" << m_source.toString() << "\"";
            }
        }
    }
    adoptImage(loaded.isNull() ? QImage() : loaded.convertToFormat(QImage::Format_ARGB32),
               Status::Error);
}

void Image::updateScaledCache(RenderMode mode) const {
    const QImage& source = effectiveImage();
    if (source.isNull()) return;

    int itemW = width();
    int itemH = height();
    if (itemW <= 0 || itemH <= 0) return;

    int targetSubW = itemW;
    int targetSubH = itemH;

    if (mode == RenderMode::HalfBlock) {
        targetSubW = itemW * half_block::SubpixelsX;
        targetSubH = itemH * half_block::SubpixelsY;
    } else if (mode == RenderMode::Braille) {
        targetSubW = itemW * braille::DotsX;
        targetSubH = itemH * braille::DotsY;
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
        double srcAspect = static_cast<double>(source.width()) / (std::max)(1, source.height());

        if (m_fillMode == FillMode::Stretch) {
            m_scaledImage = source.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectFit) {
            // Visual aspect fit: cols / (rows * 2.0) = srcAspect  =>  cols = rows * 2.0 * srcAspect
            int fitW = targetSubW;
            int fitH = std::round(targetSubW / (srcAspect * 2.0));
            if (fitH > targetSubH) {
                fitH = targetSubH;
                fitW = std::round(targetSubH * srcAspect * 2.0);
            }
            fitW = (std::clamp)(fitW, 1, targetSubW);
            fitH = (std::clamp)(fitH, 1, targetSubH);
            m_scaledImage = source.scaled(QSize(fitW, fitH), Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectCrop) {
            int cropW = targetSubW;
            int cropH = std::round(targetSubW / (srcAspect * 2.0));
            if (cropH < targetSubH) {
                cropH = targetSubH;
                cropW = std::round(targetSubH * srcAspect * 2.0);
            }
            QImage scaledTemp =
                source.scaled(QSize(cropW, cropH), Qt::IgnoreAspectRatio, transMode);
            int startX = (scaledTemp.width() - targetSubW) / 2;
            int startY = (scaledTemp.height() - targetSubH) / 2;
            m_scaledImage = scaledTemp.copy((std::max)(0, startX), (std::max)(0, startY),
                                            targetSubW, targetSubH);
        } else {
            m_scaledImage = source.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        }
    } else {
        if (m_fillMode == FillMode::Stretch) {
            m_scaledImage = source.scaled(currentTargetSize, Qt::IgnoreAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectFit) {
            m_scaledImage = source.scaled(currentTargetSize, Qt::KeepAspectRatio, transMode);
        } else if (m_fillMode == FillMode::PreserveAspectCrop) {
            QImage expanded =
                source.scaled(currentTargetSize, Qt::KeepAspectRatioByExpanding, transMode);
            int cropX = (expanded.width() - targetSubW) / 2;
            int cropY = (expanded.height() - targetSubH) / 2;
            m_scaledImage = expanded.copy(cropX, cropY, targetSubW, targetSubH);
        } else if (m_fillMode == FillMode::Pad) {
            m_scaledImage = source;
        } else if (m_fillMode == FillMode::Tile) {
            m_scaledImage = QImage(currentTargetSize, QImage::Format_ARGB32);
            m_scaledImage.fill(Qt::transparent);
            QPainter p(&m_scaledImage);
            for (int y = 0; y < targetSubH; y += source.height()) {
                for (int x = 0; x < targetSubW; x += source.width()) {
                    p.drawImage(x, y, source);
                }
            }
        }
    }
}

void Image::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    // Loading a new frame must not blank the previously committed picture.
    if (!visible() || effectiveImage().isNull()) return;

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
            if (!m_lastSixelRect.isEmpty() || m_lastSixelKey != 0)
                const_cast<Image*>(this)->releaseSixelImage();
            paintKitty(buffer, absX, absY, clipRect);
            return;
        }
        if (use == graphics::Protocol::Sixel) {
            paintSixel(buffer, absX, absY, clipRect);
            return;
        }
        mode = RenderMode::HalfBlock;
    }
    if (!m_lastSixelRect.isEmpty() || m_lastSixelKey != 0)
        const_cast<Image*>(this)->releaseSixelImage();

    updateScaledCache(mode);
    if (m_scaledImage.isNull()) return;

    int subW = m_scaledImage.width();
    int subH = m_scaledImage.height();

    if (mode == RenderMode::HalfBlock) {
        // Compute alignment offsets in subpixel space
        int subTargetW = itemW * half_block::SubpixelsX;
        int subTargetH = itemH * half_block::SubpixelsY;

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

                if (!clipRect.contains(targetX, targetY)) continue;

                int topPy = cy * half_block::SubpixelsY - offsetY;
                int botPy = cy * half_block::SubpixelsY + 1 - offsetY;
                int px = cx - offsetX;

                const QRgb top = (px >= 0 && px < subW && topPy >= 0 && topPy < subH)
                                     ? m_scaledImage.pixel(px, topPy)
                                     : 0u;
                const QRgb bot = (px >= 0 && px < subW && botPy >= 0 && botPy < subH)
                                     ? m_scaledImage.pixel(px, botPy)
                                     : 0u;

                Cell* targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell) continue;
                Cell& cell = *targetCell;
                const uint32_t behind = cellBackgroundRgb(cell.bgColor);
                const uint32_t topShown = compositeOver(top, behind);
                const uint32_t botShown = compositeOver(bot, behind);
                if (topShown == 0 && botShown == 0)
                    continue; // both halves clear: the cell keeps its glyph and colours

                cell.displayWidth = 1;
                cell.continuation = false;
                // The glyph covers only the painted half, so a clear half shows
                // the background behind the image rather than a stale foreground.
                if (topShown == 0) {
                    cell.setCharacter(block_glyphs::LowerHalf); // ▄ Lower Half Block
                    cell.fgColor = botShown;
                } else {
                    cell.setCharacter(block_glyphs::UpperHalf); // ▀ Upper Half Block
                    cell.fgColor = topShown;
                    if (botShown != 0) cell.bgColor = botShown;
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

                if (!clipRect.contains(targetX, targetY)) continue;

                int px = cx - offsetX;
                int py = cy - offsetY;

                if (px < 0 || px >= subW || py < 0 || py >= subH) continue;

                Cell* targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell) continue;
                Cell& cell = *targetCell;
                // Glyph and colour both follow the composited pixel, so a faint
                // pixel over a dark background reads as a faint glyph.
                const uint32_t shown =
                    compositeOver(m_scaledImage.pixel(px, py), cellBackgroundRgb(cell.bgColor));
                if (shown == 0) continue;

                const int brightness = qGray(shown);
                const int rampIndex = (brightness * (ASCII_RAMP_LEN - 1)) / 255;
                cell.displayWidth = 1;
                cell.continuation = false;
                const char asciiChar[2] = {ASCII_RAMP[rampIndex], '\0'};
                cell.setCharacter(asciiChar);
                cell.fgColor = shown;
            }
        }
    } else if (mode == RenderMode::Braille) {
        // 2x4 subpixels per cell
        int subTargetW = itemW * braille::DotsX;
        int subTargetH = itemH * braille::DotsY;

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

                if (!clipRect.contains(targetX, targetY)) continue;

                Cell* targetCell = buffer.tryCellAt(targetX, targetY);
                if (!targetCell) continue;
                const uint32_t behind = cellBackgroundRgb(targetCell->bgColor);

                uint8_t cellValue = 0;
                uint32_t lastRgb = 0xFFFFFFFF;

                for (int dotY = 0; dotY < braille::DotsY; ++dotY) {
                    for (int dotX = 0; dotX < braille::DotsX; ++dotX) {
                        int px = cx * braille::DotsX + dotX - offsetX;
                        int py = cy * braille::DotsY + dotY - offsetY;
                        if (px < 0 || px >= subW || py < 0 || py >= subH) continue;
                        // A dot is set when the composited pixel clears the dark
                        // cut-off, so a faint pixel over a dark background stays off.
                        const uint32_t shown = compositeOver(m_scaledImage.pixel(px, py), behind);
                        if (shown != 0 && qGray(shown) > 50) {
                            cellValue |= braille::dotMask(dotX, dotY);
                            lastRgb = shown;
                        }
                    }
                }

                if (cellValue > 0) {
                    Cell& cell = *targetCell;
                    cell.displayWidth = 1;
                    cell.continuation = false;
                    cell.setCharacter(braille::utf8(cellValue));
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
    const QImage& source = effectiveImage();
    if (itemW <= 0 || itemH <= 0 || source.isNull()) return;

    const int sw = source.width();
    const int sh = source.height();
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
        cols = (std::clamp)(fitCols, 1, itemW);
        rows = (std::clamp)(fitRows, 1, itemH);
        if (m_alignment & Qt::AlignHCenter)
            originX += (itemW - cols) / 2;
        else if (m_alignment & Qt::AlignRight)
            originX += itemW - cols;
        if (m_alignment & Qt::AlignVCenter)
            originY += (itemH - rows) / 2;
        else if (m_alignment & Qt::AlignBottom)
            originY += itemH - rows;
    }

    // Re-upload only when the source, the upload size or session changed.
    // The placement is updated separately when placement geometry changes.
    const qint64 key = source.cacheKey();
    const QSize cellPx = root->capabilities().cellPixelSize;
    const QSize placementPx =
        cellPx.isValid() ? QSize(cols * cellPx.width(), rows * cellPx.height()) : QSize();
    const QSize uploadSize = graphics::kittyUploadSize(source.size(), placementPx, m_smooth);
    const quint64 session = root->engine() ? root->engine()->outputSessionGeneration() : 0;

    if (m_imageId == 0) m_imageId = graphics::nextImageId();

    const bool needUpload =
        (key != m_lastKittyKey || uploadSize != m_lastKittyUpload || session != m_lastKittySession);
    if (needUpload) {
        QByteArray wire;
        const bool cachePayload = !m_sourceClipRect.isNull();
        if (cachePayload) {
            for (const KittyPayloadCacheEntry& entry : std::as_const(m_kittyPayloadCache)) {
                if (entry.frameKey == key && entry.uploadSize == uploadSize &&
                    entry.imageId == m_imageId) {
                    wire = entry.payload;
                    break;
                }
            }
        }
        if (wire.isEmpty()) {
            QImage upload = source;
            if (uploadSize != source.size())
                upload = upload.scaled(uploadSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            wire = graphics::kittyTransmit(m_imageId, upload);
            if (cachePayload && !wire.isEmpty()) {
                constexpr qsizetype kMaxKittyCache = 16;
                if (m_kittyPayloadCache.size() >= kMaxKittyCache) m_kittyPayloadCache.removeFirst();
                m_kittyPayloadCache.append(
                    KittyPayloadCacheEntry{key, uploadSize, m_imageId, wire});
            }
        }
        if (!root->enqueueControlSequence(wire)) return;
        m_lastKittyUpload = uploadSize;
        m_lastKittyKey = key;
        m_lastKittySession = session;
    }

    if (needUpload || QSize(cols, rows) != m_lastKittyCells) {
        if (!root->enqueueControlSequence(graphics::kittyPlacement(m_imageId, cols, rows))) return;
        m_lastKittyCells = QSize(cols, rows);
    }

    graphics::writeKittyPlaceholders(buffer, originX, originY, cols, rows, m_imageId, clipRect);
}

void Image::paintSixel(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    Terminal* root = terminal();
    if (!root) return;
    const int itemW = width();
    const int itemH = height();
    const QImage& source = effectiveImage();
    if (itemW <= 0 || itemH <= 0 || source.isNull()) return;
    const int sw = source.width();
    const int sh = source.height();
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
        cols = (std::clamp)(fitCols, 1, itemW);
        rows = (std::clamp)(fitRows, 1, itemH);
        if (m_alignment & Qt::AlignHCenter)
            originX += (itemW - cols) / 2;
        else if (m_alignment & Qt::AlignRight)
            originX += itemW - cols;
        if (m_alignment & Qt::AlignVCenter)
            originY += (itemH - rows) / 2;
        else if (m_alignment & Qt::AlignBottom)
            originY += itemH - rows;
    }

    // Reserve the cells so the diff clears whatever was there and nothing else
    // paints into the image area. The sixel is drawn on top, after the diff.
    bool anyVisibleCell = false;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int x = originX + c;
            const int y = originY + r;
            if (!clipRect.contains(x, y)) continue;
            Cell* target = buffer.tryCellAt(x, y);
            if (!target) continue;
            anyVisibleCell = true;
            target->setCharacter(" ");
            target->displayWidth = 1;
            target->continuation = false;
            target->fgColor = 0x00000000u;
            target->bgColor = 0x00000000u;
        }
    }
    if (!anyVisibleCell) return;

    const int pixelW = (std::min)(4096, cols * cw);
    const int pixelH = (std::min)(4096, rows * ch);
    if (pixelW <= 0 || pixelH <= 0) return;

    // Re-emit when the source, placement, pixel size, filtering, viewport,
    // frame continuity, or output session changed. Skipping a frame (mode
    // switch, hidden ancestor) or starting a new session means the sixel on
    // screen was overwritten or cleared and must be re-sent.
    const QRect placementRect(originX, originY, cols, rows);
    const QSize pixelSize(pixelW, pixelH);
    const QSize viewportSize(buffer.width(), buffer.height());
    const quint64 session = root->engine() ? root->engine()->outputSessionGeneration() : 0;
    const quint64 frameSeq = root->engine() ? root->engine()->frameSequence() : 0;
    const bool consecutiveFrame =
        m_hasLastSixelFrame && (frameSeq == m_lastSixelFrame + 1 || frameSeq == m_lastSixelFrame);
    m_lastSixelFrame = frameSeq;
    m_hasLastSixelFrame = true;

    const qint64 key = source.cacheKey();
    if (consecutiveFrame && key == m_lastSixelKey && placementRect == m_lastSixelRect &&
        pixelSize == m_lastSixelPixels && viewportSize == m_lastSixelViewport &&
        m_smooth == m_lastSixelSmooth && session == m_lastSixelSession) {
        m_paintedSixelThisFrame = true;
        return;
    }

    if (!m_lastSixelRect.isEmpty() && m_lastSixelRect != placementRect) {
        const_cast<Image*>(this)->releaseSixelImage();
        m_lastSixelFrame = frameSeq;
        m_hasLastSixelFrame = true;
    }

    QImage scaled = source.scaled(pixelSize, Qt::IgnoreAspectRatio,
                                  m_smooth ? Qt::SmoothTransformation : Qt::FastTransformation);
    const QByteArray sixel = graphics::sixelEncode(scaled);
    if (sixel.isEmpty()) return;

    m_lastSixelKey = key;
    m_lastSixelPixels = pixelSize;
    m_lastSixelViewport = viewportSize;
    m_lastSixelSmooth = m_smooth;
    m_lastSixelSession = session;

    // Clear the placement rectangle first. Sixel with P2=1 leaves unset pixels
    // untouched, so without an explicit clear the previous frame's opaque
    // pixels accumulate under the new one. An earlier placement at a different
    // geometry was already erased above before the cell diff.
    QByteArray positioned = "\033[0m\0337"; // default SGR, save cursor
    const QByteArray blank(cols, ' ');
    for (int r = 0; r < rows; ++r) {
        const int rowCoord = (std::max)(1, originY + 1 + r);
        const int colCoord = (std::max)(1, originX + 1);
        positioned += "\033[" + QByteArray::number(rowCoord) + ';' + QByteArray::number(colCoord) +
                      'H' + blank;
    }
    const int origRowCoord = (std::max)(1, originY + 1);
    const int origColCoord = (std::max)(1, originX + 1);
    positioned += "\033[" + QByteArray::number(origRowCoord) + ';' +
                  QByteArray::number(origColCoord) + 'H'; // move to the image origin
    positioned += sixel;
    positioned += "\0338"; // restore cursor
    root->enqueuePostControlSequence(positioned);
    m_lastSixelRect = placementRect;
    m_sixelTerminal = root;
    m_paintedSixelThisFrame = true;
    if (TerminalEngine* engine = root->engine(); engine && !m_sixelPaintConnection) {
        auto* self = const_cast<Image*>(this);
        m_sixelPaintConnection = connect(engine, &TerminalEngine::framePainted, self, [self] {
            if (self->m_paintedSixelThisFrame) {
                self->m_paintedSixelThisFrame = false;
                return;
            }
            self->releaseSixelImage();
        });
    }
}

} // namespace qmltui
