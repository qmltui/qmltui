#pragma once

#include <Qamui/Core/primitives/QamuiExport.h>

#include <Qamui/Core/items/Item.h>
#include <Qamui/Image/TerminalGraphics.h>
#include <QMetaObject>
#include <QPointer>
#include <QImage>
#include <QUrl>
#include <QSize>
#include <QRect>

class QNetworkReply;

namespace qamui {

class ImageResponse;

class QAMUI_IMAGE_EXPORT Image : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    // The picture itself. Assign a QImage from C++ (or bind to a QImage
    // property of a C++ object) to show pixels that never touch a file; the
    // item then has no `source`. Reading returns whatever is shown, loaded or
    // assigned. A null image clears the item.
    Q_PROPERTY(QImage image READ image WRITE setImage NOTIFY imageChanged)
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
    Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode NOTIFY renderModeChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    // Keep the previous picture visible during an asynchronous source change.
    // Opt-in, matching Qt Quick Image; useful for live image providers.
    Q_PROPERTY(bool retainWhileLoading READ retainWhileLoading WRITE setRetainWhileLoading NOTIFY retainWhileLoadingChanged)
    // How much of a remote (http, https) source has arrived, 0.0 to 1.0; 1.0
    // once any picture is ready, 0.0 while nothing is shown.
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    // The sub-rectangle of the source, in source pixels, that this item shows.
    // A null rectangle (the default) shows the whole picture. The clip crops
    // before any scaling, so fillMode, alignment and the pixel protocols see
    // only the clipped region; the implicit size follows the clip while
    // sourceSize keeps reporting the full picture. Out-of-bounds parts are
    // cut off; a clip entirely outside the picture draws nothing.
    Q_PROPERTY(QRect sourceClipRect READ sourceClipRect WRITE setSourceClipRect RESET resetSourceClipRect NOTIFY sourceClipRectChanged)
    // Nearest-neighbour sampling by default keeps cell-sized edges crisp.
    // Opt in to filtering when softer transitions are wanted.
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    // The pixel grid this item paints into, so a producer can draw frames at
    // exactly the resolution the terminal will show: columns times rows times
    // the cell's pixel size when kitty or sixel will be used, otherwise the
    // glyph grid (1x2 per cell for half-block, 2x4 for Braille, 1x1 for
    // ASCII). Invalid while the item has no size. Follows the item size, the
    // render mode and the terminal's capabilities.
    Q_PROPERTY(QSize renderSize READ renderSize NOTIFY renderSizeChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)

public:
    enum class FillMode : uint8_t {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop,
        Tile,
        Pad,
    };
    Q_ENUM(FillMode)

    enum class RenderMode : uint8_t {
        HalfBlock,  // 1x2 TrueColor subpixels per cell using ▀
        Braille,    // 2x4 dot subpixel grid
        ASCII,      // Brightness gradient mapping to ASCII characters
        Kitty,      // true pixels via the kitty graphics protocol
        Sixel,      // true pixels via the sixel protocol (e.g. Windows Terminal)
        Auto,       // best pixel protocol the terminal supports, else HalfBlock
    };
    Q_ENUM(RenderMode)

    enum class Status : uint8_t {
        Null,
        Ready,
        Loading,
        Error,
    };
    Q_ENUM(Status)

    explicit Image(QObject* parent = nullptr);
    ~Image() override;

    QUrl source() const { return m_source; }
    void setSource(const QUrl& source);

    QImage image() const { return m_sourceImage; }
    void setImage(const QImage& image);

    FillMode fillMode() const { return m_fillMode; }
    void setFillMode(FillMode mode);

    RenderMode renderMode() const { return m_renderMode; }
    void setRenderMode(RenderMode mode);

    Status status() const { return m_status; }
    bool retainWhileLoading() const { return m_retainWhileLoading; }
    void setRetainWhileLoading(bool retain);
    qreal progress() const { return m_progress; }

    QSize sourceSize() const { return m_sourceImage.size(); }

    QRect sourceClipRect() const { return m_sourceClipRect; }
    void setSourceClipRect(const QRect& rect);
    void resetSourceClipRect() { setSourceClipRect(QRect()); }

    bool smooth() const { return m_smooth; }
    void setSmooth(bool smooth);

    QSize renderSize() const;

    Qt::Alignment alignment() const { return m_alignment; }
    void setAlignment(Qt::Alignment alignment);

    int implicitWidth() const override;
    int implicitHeight() const override;

    void paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) const override;

signals:
    void sourceChanged();
    void imageChanged();
    void fillModeChanged();
    void renderModeChanged();
    void statusChanged();
    void retainWhileLoadingChanged();
    void progressChanged();
    void sourceSizeChanged();
    void sourceClipRectChanged();
    void smoothChanged();
    void alignmentChanged();
    void renderSizeChanged();

private:
    void loadImage();
    // Loads an image:// source through the ImageProvider registered on this
    // item's QML engine under the URL's host. Returns false when no provider
    // exists; sets *pending when an asynchronous provider took the request.
    QImage loadFromProvider(const QUrl& url, bool* found, bool* pending);
    // Drops interest in an outstanding asynchronous response.
    void cancelPendingResponse();
    // Fetches an http(s) source through the QML engine's network access
    // manager (a shared one without an engine). Loading until the reply
    // finishes; retainWhileLoading can keep the current picture meanwhile. Decoded
    // pictures are cached by URL so a relayout or a second item does not
    // refetch.
    void loadFromNetwork(const QUrl& url);
    // Aborts and forgets an outstanding download.
    void cancelPendingReply();
    void setProgress(qreal progress);
    // Installs a decoded picture (or a null one) as the source and publishes
    // the status, size and geometry changes it implies.
    void adoptImage(const QImage& image, Status statusIfNull);
    // Scales the source for the mode that will actually paint. The declared
    // mode may be Auto, Kitty or Sixel and fall back to HalfBlock; the cell
    // geometry (2 pixel rows per cell, 2x4 for Braille, 1x1 for ASCII) must
    // follow the resolved mode or the picture is fitted to the wrong grid.
    void updateScaledCache(RenderMode mode) const;
    // Emits transmission, placement and placeholder cells for the kitty
    // protocol; replaces pixels under the same id when the source changes.
    void paintKitty(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const;
    // Reserves the image cells and enqueues a positioned sixel after the diff.
    void paintSixel(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const;
    // Frees the current kitty upload if the terminal is still reachable.
    void releaseKittyImage();
    // Erases the persisted sixel when the picture is discarded.
    void releaseSixelImage();
    // The pixel protocol paint() will use for the current mode and terminal:
    // None means a glyph mode (half-block, Braille or ASCII).
    graphics::Protocol activeProtocol() const;
    // Re-emits renderSizeChanged() when the terminal's capabilities change.
    void watchCapabilities() const;
    // The picture the paint paths consume: the source itself, or, with a
    // sourceClipRect, a cached copy of the clipped region. Null when the clip
    // falls entirely outside the picture.
    const QImage& effectiveImage() const;

    QUrl m_source;
    FillMode m_fillMode = FillMode::Stretch;
    RenderMode m_renderMode = RenderMode::Auto;
    Status m_status = Status::Null;
    bool m_retainWhileLoading = false;
    bool m_smooth = false;
    Qt::Alignment m_alignment = Qt::AlignCenter;

    QImage m_sourceImage;
    QRect m_sourceClipRect;
    // Copy of the clipped region, refreshed lazily by effectiveImage().
    mutable QImage m_clippedImage;
    mutable qint64 m_clippedSourceKey = 0;
    mutable QRect m_clippedForRect;
    // Scaled copy of the source for the current size: a cache refreshed while
    // painting, so it is mutable.
    mutable QImage m_scaledImage;
    mutable QSize m_lastTargetSize;
    mutable RenderMode m_lastRenderMode = RenderMode::HalfBlock;
    mutable FillMode m_lastFillMode = FillMode::Stretch;
    mutable bool m_lastSmooth = false;

    // kitty graphics state. m_imageId is 0 until the first upload.
    mutable quint32 m_imageId = 0;
    mutable qint64 m_lastKittyKey = 0;
    mutable QSize m_lastKittyUpload;   // pixel size of the current upload
    mutable quint64 m_lastKittySession = 0;
    QPointer<ImageResponse> m_pendingResponse;
    QPointer<QNetworkReply> m_reply;
    qreal m_progress = 0.0;
    mutable QPointer<Terminal> m_watchedTerminal;
    mutable QMetaObject::Connection m_capabilitiesConnection;
    mutable QSize m_lastKittyCells;

    // sixel state. m_lastSixelSignature encodes source, geometry and output
    // session so the image is re-emitted after any of them change.
    mutable qint64 m_lastSixelKey = 0;
    mutable QSize m_lastSixelCells;
    mutable quint64 m_lastSixelSession = 0;
    mutable QRect m_lastSixelRect;
};

} // namespace qamui
