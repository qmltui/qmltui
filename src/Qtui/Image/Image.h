#pragma once

#include "Item.h"
#include <QImage>
#include <QUrl>
#include <QSize>
#include <QColor>

namespace qtui {

class Image : public Item {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
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
        ASCII       // Brightness gradient mapping to ASCII characters
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
    ~Image() override = default;

    QUrl source() const { return m_source; }
    void setSource(const QUrl& source);

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

    void render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect = ClipRect()) override;

signals:
    void sourceChanged();
    void fillModeChanged();
    void renderModeChanged();
    void statusChanged();
    void sourceSizeChanged();
    void smoothChanged();
    void alignmentChanged();

private:
    void loadImage();
    void updateScaledCache();

    QUrl m_source;
    FillMode m_fillMode = FillMode::Stretch;
    RenderMode m_renderMode = RenderMode::HalfBlock;
    Status m_status = Status::Null;
    bool m_smooth = true;
    Qt::Alignment m_alignment = Qt::AlignCenter;

    QImage m_sourceImage;
    QImage m_scaledImage;
    QSize m_lastTargetSize;
    FillMode m_lastFillMode = FillMode::Stretch;
    bool m_lastSmooth = true;
};

} // namespace qtui
