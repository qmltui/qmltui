#pragma once

#include <Qtui/Core/primitives/QtuiExport.h>

#include <Qtui/Core/items/Item.h>
#include <QImage>
#include <QUrl>
#include <QSize>

namespace qtui {

class QTUI_IMAGE_EXPORT Image : public Item {
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
    Q_PROPERTY(QSize sourceSize READ sourceSize NOTIFY sourceSizeChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)

public:
    enum class FillMode {
        Stretch,
        PreserveAspectFit,
        PreserveAspectCrop,
        Tile,
        Pad
    };
    Q_ENUM(FillMode)

    enum class RenderMode {
        HalfBlock,  // 1x2 TrueColor subpixels per cell using ▀
        Braille,    // 2x4 dot subpixel grid
        ASCII,      // Brightness gradient mapping to ASCII characters
        Kitty,      // true pixels via the kitty graphics protocol
        Sixel,      // true pixels via the sixel protocol (e.g. Windows Terminal)
        Auto        // best pixel protocol the terminal supports, else HalfBlock
    };
    Q_ENUM(RenderMode)

    enum class Status {
        Null,
        Ready,
        Loading,
        Error
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

    QSize sourceSize() const { return m_sourceImage.size(); }

    bool smooth() const { return m_smooth; }
    void setSmooth(bool smooth);

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
    void sourceSizeChanged();
    void smoothChanged();
    void alignmentChanged();

private:
    void loadImage();
    // Loads an image:// source through the ImageProvider registered on this
    // item's QML engine under the URL's host.
    QImage loadFromProvider(const QUrl& url, bool* found) const;
    // Installs a decoded picture (or a null one) as the source and publishes
    // the status, size and geometry changes it implies.
    void adoptImage(const QImage& image, Status statusIfNull);
    // Scales the source for the mode that will actually paint. The declared
    // mode may be Auto, Kitty or Sixel and fall back to HalfBlock; the cell
    // geometry (2 pixel rows per cell, 2x4 for Braille, 1x1 for ASCII) must
    // follow the resolved mode or the picture is fitted to the wrong grid.
    void updateScaledCache(RenderMode mode) const;
    // Emits transmission, placement and placeholder cells for the kitty
    // protocol; frees the previous upload when the source or geometry changes.
    void paintKitty(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const;
    // Reserves the image cells and enqueues a positioned sixel after the diff.
    void paintSixel(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const;
    // Frees the current kitty upload if the terminal is still reachable.
    void releaseKittyImage() const;

    QUrl m_source;
    FillMode m_fillMode = FillMode::Stretch;
    RenderMode m_renderMode = RenderMode::Auto;
    Status m_status = Status::Null;
    bool m_smooth = true;
    Qt::Alignment m_alignment = Qt::AlignCenter;

    QImage m_sourceImage;
    // Scaled copy of the source for the current size: a cache refreshed while
    // painting, so it is mutable.
    mutable QImage m_scaledImage;
    mutable QSize m_lastTargetSize;
    mutable RenderMode m_lastRenderMode = RenderMode::HalfBlock;
    mutable FillMode m_lastFillMode = FillMode::Stretch;
    mutable bool m_lastSmooth = true;

    // kitty graphics state. m_imageId is 0 until the first upload.
    mutable quint32 m_imageId = 0;
    mutable qint64 m_lastKittyKey = 0;
    mutable QSize m_lastKittyUpload;   // pixel size of the current upload
    mutable QSize m_lastKittyCells;

    // sixel state. m_lastSixelSignature encodes source, geometry and output
    // session so the image is re-emitted after any of them change.
    mutable qint64 m_lastSixelKey = 0;
    mutable QSize m_lastSixelCells;
    mutable quint64 m_lastSixelSession = 0;
};

} // namespace qtui
