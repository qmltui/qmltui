// Kitty graphics output from the Image item, verified through the memory
// backend: transmission and placement precede the placeholder cells, the
// cells carry the image id in their foreground, source changes reuse that id,
// an unchanged frame re-sends nothing, and Auto falls back to
// half-block on a terminal without the protocol.
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QImage>
#include <QRegularExpression>
#include <QMap>
#include <QtEndian>

#include <Qamui/Runner/MemoryTerminalBackend.h>
#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Core/engine/TerminalEngine.h>
#include <Qamui/Image/Image.h>
#include <Qamui/Image/ImageProvider.h>
#include <Qamui/Image/TerminalGraphics.h>
#include <QSignalSpy>
#include <Qamui/Core/primitives/TextGridBuffer.h>

using namespace qamui;

namespace {
MemoryTerminalBackend* backend = nullptr;
constexpr char kPlaceholderUtf8[4] = { '\xF4', '\x8E', '\xBB', '\xAE' };  // U+10EEEE

int placeholderCellCount(TerminalEngine* engine, uint32_t* fgOut = nullptr) {
    const auto frame = engine->currentFrame();
    if (!frame) return 0;
    int count = 0;
    for (int y = 0; y < frame->height(); ++y)
        for (int x = 0; x < frame->width(); ++x) {
            const QByteArray g = frame->cellAt(x, y).character().toByteArray();
            if (g.startsWith(QByteArray(kPlaceholderUtf8, 4))) {
                if (fgOut) *fgOut = frame->cellAt(x, y).fgColor;
                ++count;
            }
        }
    return count;
}

QString makePng(const QString& dir, const QString& name, QColor color) {
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(color);
    const QString path = dir + '/' + name;
    image.save(path, "PNG");
    return path;
}

// Hold each replacement until the test has inspected intervening frames.
class PendingResponse : public ImageResponse {
public:
    QImage image() const override { return m_image; }
    void complete(const QImage& image) { m_image = image; emit finished(); }
private:
    QImage m_image;
};

class PendingProvider : public AsyncImageProvider {
public:
    QPointer<PendingResponse> pending;
    ImageResponse* requestImageResponse(const QString&, const QSize&) override {
        pending = new PendingResponse;
        return pending;
    }
};
}

class TestKittyGraphics : public QObject {
    Q_OBJECT
    QTemporaryDir dir;
    QString redPng, bluePng;

    Terminal* buildTerminal(QQmlEngine& qml, std::unique_ptr<QObject>& holder,
                            const TerminalCapabilities& caps) {
        QQmlComponent component(&qml);
        component.setData(R"(
import QtQml
import Qamui
import Qamui.Image
Terminal {
    Image { objectName: "img"; x: 0; y: 0; width: 6; height: 3
            fillMode: Image.Stretch; renderMode: Image.Auto }
}
)", QUrl());
        holder.reset(component.create());
        if (!holder) { qWarning() << component.errorString(); return nullptr; }
        auto* terminal = qobject_cast<Terminal*>(holder.get());
        terminal->engine()->setCapabilities(caps);   // deterministic; no probes
        return terminal;
    }
    static TerminalCapabilities kittyCaps() {
        TerminalCapabilities caps;
        caps.colorDepth = ColorDepth::TrueColor;
        caps.kittyGraphics = true;
        caps.cellPixelSize = QSize(10, 20);
        return caps;
    }
    static TerminalCapabilities sixelCaps() {
        TerminalCapabilities caps;
        caps.colorDepth = ColorDepth::TrueColor;
        caps.sixelGraphics = true;
        caps.cellPixelSize = QSize(10, 20);
        return caps;
    }

private slots:
    // Reassembles compressed RGBA from the transmit (`m=1` chunks then `m=0`)
    // that follows the first transmit command in `out`.
    static QImage transmittedImage(const std::string& out) {
        size_t pos = out.find("\033_Gq=2,a=t");
        if (pos == std::string::npos) return {};
        const QRegularExpression dimensions("s=(\\d+),v=(\\d+)");
        const auto match = dimensions.match(QString::fromStdString(out.substr(pos, out.find(';', pos) - pos)));
        if (!match.hasMatch()) return {};
        const int width = match.captured(1).toInt();
        const int height = match.captured(2).toInt();
        QByteArray base64;
        while (pos != std::string::npos) {
            const size_t semicolon = out.find(';', pos);
            const size_t end = out.find("\033\\", pos);
            if (semicolon == std::string::npos || end == std::string::npos) break;
            if (end - semicolon - 1 > 4096) return {};
            const std::string header = out.substr(pos, semicolon - pos);
            base64 += QByteArray(out.data() + semicolon + 1, int(end - semicolon - 1));
            if (header.find("m=1") == std::string::npos) break;   // last chunk
            pos = out.find("\033_G", end);
        }
        QByteArray compressed(4, '\0');
        qToBigEndian<quint32>(width * height * 4, compressed.data());
        compressed += QByteArray::fromBase64(base64);
        const QByteArray pixels = qUncompress(compressed);
        if (pixels.size() != qsizetype(width) * height * 4) return {};
        return QImage(reinterpret_cast<const uchar*>(pixels.constData()), width, height,
                      QImage::Format_RGBA8888).copy();
    }

    void transmissionPreservesPixelsAndChunks() {
        QVERIFY(graphics::kittyTransmit(1, QImage()).isEmpty());
        QImage source(127, 65, QImage::Format_ARGB32);
        quint32 random = 12345;
        for (int y = 0; y < source.height(); ++y)
            for (int x = 0; x < source.width(); ++x) {
                random = random * 1664525u + 1013904223u;
                source.setPixel(x, y, random); // distinct channels, including partial alpha
            }
        const QByteArray encoded = graphics::kittyTransmit(42, source);
        QVERIFY(encoded.startsWith("\033_Gq=2,a=t,f=32,o=z,t=d,i=42,s=127,v=65,m=1;"));
        QVERIFY(encoded.contains("\033_Gm=0;"));
        QCOMPARE(transmittedImage(encoded.toStdString()).convertToFormat(QImage::Format_ARGB32), source);

        // Byte-order conversion and externally supplied scanline padding.
        const QImage rgba = source.convertToFormat(QImage::Format_RGBA8888);
        const int stride = source.width() * 4 + 12;
        QByteArray storage(stride * source.height(), '\x55');
        for (int y = 0; y < source.height(); ++y)
            memcpy(storage.data() + stride * y, rgba.constScanLine(y), source.width() * 4);
        const QImage padded(reinterpret_cast<const uchar*>(storage.constData()),
                            source.width(), source.height(), stride, QImage::Format_RGBA8888);
        QCOMPARE(transmittedImage(graphics::kittyTransmit(43, padded).toStdString()), rgba);
    }

    void initTestCase() {
        QVERIFY(dir.isValid());
        redPng = makePng(dir.path(), "red.png", Qt::red);
        bluePng = makePng(dir.path(), "blue.png", Qt::blue);
    }
    void init() {
        TerminalBackend::setFactory([](int) {
            auto memory = std::make_unique<MemoryTerminalBackend>(QSize(20, 6));
            backend = memory.get();
            return memory;
        });
    }
    void cleanup() { TerminalBackend::setFactory(nullptr); }

    void transmitPlaceThenCells() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        QVERIFY(img);
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(placeholderCellCount(engine) > 0);

        const std::string& out = backend->output();
        const auto transmit = out.find("\033_Gq=2,a=t,f=32,o=z,t=d,i=");
        const auto place = out.find("\033_Ga=p,U=1,q=2,i=");
        QVERIFY(transmit != std::string::npos);
        QVERIFY(place != std::string::npos);
        QVERIFY(transmit < place);   // upload before placement

        // The placement id matches the id carried by the placeholder cells.
        const QRegularExpression re("\\033_Ga=p,U=1,q=2,i=(\\d+),p=1,c=6,r=3");
        const auto m = re.match(QString::fromLatin1(out.data(), int(out.size())));
        QVERIFY2(m.hasMatch(), "placement not found with the expected cols/rows");
        const uint32_t id = m.captured(1).toUInt();
        uint32_t fg = 0;
        QCOMPARE(placeholderCellCount(engine, &fg), 18);   // 6x3 cells
        QCOMPARE(fg, 0xFF000000u | (id & 0xFFFFFFu));
        QVERIFY(place < out.find(std::string(kPlaceholderUtf8, 4)));  // cells after controls
    }

    // The upload is pre-scaled to the placement's pixel size when a small
    // source would be magnified and `smooth` is set, so the terminal does not
    // nearest-neighbour it into blocks; without `smooth` the source goes as is.
    void magnifiedUploadIsPrescaledWhenSmooth() {
        using graphics::kittyUploadSize;
        QCOMPARE(kittyUploadSize(QSize(60, 60), QSize(220, 240), true), QSize(220, 240));
        QCOMPARE(kittyUploadSize(QSize(60, 60), QSize(220, 240), false), QSize(60, 60));
        QCOMPARE(kittyUploadSize(QSize(600, 600), QSize(220, 240), true), QSize(600, 600));   // shrinking: terminal scales
        QCOMPARE(kittyUploadSize(QSize(60, 60), QSize(), true), QSize(60, 60));               // cell size unknown
        QCOMPARE(kittyUploadSize(QSize(60, 60), QSize(4000, 4000), true), QSize(2048, 2048)); // capped
        QCOMPARE(kittyUploadSize(QSize(3000, 1500), QSize(), true), QSize(2048, 1024));

        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());   // 10x20 px cells, 6x3 item
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(placeholderCellCount(engine) > 0);
        const QSize source = QImage(redPng).size();
        QVERIFY(source.width() < 60);   // the fixture picture is small: this placement magnifies it
        QCOMPARE(img->smooth(), false);
        QCOMPARE(transmittedImage(backend->output()).size(), source);

        backend->clearOutput();
        img->setSmooth(true);
        QTRY_VERIFY(backend->output().find("\033_Gq=2,a=t") != std::string::npos);
        QCOMPARE(transmittedImage(backend->output()).size(), QSize(60, 60));   // 6 cols x 10 px, 3 rows x 20 px

        backend->clearOutput();
        img->setSmooth(false);            // pixel art: let the terminal magnify the blocks
        QTRY_VERIFY(backend->output().find("\033_Gq=2,a=t") != std::string::npos);
        QCOMPARE(transmittedImage(backend->output()).size(), source);
    }

    // renderSize is the pixel grid the item paints into: true pixels when a
    // protocol is active, the glyph grid otherwise; it follows mode, size and
    // capabilities, and a provider receives it as the requested size.
    void renderSizeFollowsProtocol() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());   // 10x20 px cells, 6x3 item, Auto
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        QSignalSpy changed(img, &Image::renderSizeChanged);
        QCOMPARE(img->renderSize(), QSize(60, 60));
        img->setRenderMode(Image::RenderMode::Braille);
        QCOMPARE(img->renderSize(), QSize(12, 12));
        img->setRenderMode(Image::RenderMode::ASCII);
        QCOMPARE(img->renderSize(), QSize(6, 3));
        img->setRenderMode(Image::RenderMode::HalfBlock);
        QCOMPARE(img->renderSize(), QSize(6, 6));
        img->setRenderMode(Image::RenderMode::Kitty);
        QCOMPARE(img->renderSize(), QSize(60, 60));
        QCOMPARE(changed.count(), 4);
        img->setWidth(8);
        QCOMPARE(img->renderSize(), QSize(80, 60));
        QCOMPARE(changed.count(), 5);
        TerminalCapabilities none;
        none.colorDepth = ColorDepth::TrueColor;
        terminal->engine()->setCapabilities(none);      // the terminal lost its protocol
        QCOMPARE(img->renderSize(), QSize(8, 6));        // Kitty falls back to half-block
        QVERIFY(changed.count() >= 6);
        terminal->engine()->setCapabilities(sixelCaps());
        img->setRenderMode(Image::RenderMode::Auto);
        QCOMPARE(img->renderSize(), QSize(80, 60));
        TerminalCapabilities huge = kittyCaps();
        huge.cellPixelSize = QSize(1920, 1080);          // a window size, not a cell size
        terminal->engine()->setCapabilities(huge);
        const QSize clamped = img->renderSize();
        QCOMPARE(clamped, graphics::kittyUploadSize(QSize(8 * 1920, 3 * 1080), QSize(), false));
        QVERIFY(clamped.width() <= 2048 && clamped.height() <= 2048);
    }

    void unchangedFrameResendsNothing() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(placeholderCellCount(engine) > 0);
        backend->clearOutput();
        const auto seq = engine->frameSequence();
        engine->requestRender();
        QTRY_VERIFY(engine->frameSequence() > seq);
        QVERIFY(backend->output().find("\033_G") == std::string::npos);
    }

    void sourceChangeReusesUploadId() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(placeholderCellCount(engine) > 0);
        const QRegularExpression re("\\033_Ga=p,U=1,q=2,i=(\\d+)");
        const uint32_t firstId =
            re.match(QString::fromLatin1(backend->output().data(), int(backend->output().size())))
              .captured(1).toUInt();
        backend->clearOutput();
        img->setSource(QUrl::fromLocalFile(bluePng));
        QTRY_VERIFY(backend->output().find("\033_Ga=p") != std::string::npos);
        const std::string& out = backend->output();
        QVERIFY(out.find("\033_Ga=d") == std::string::npos);
        QVERIFY(out.find("\033_Gq=2,a=t") != std::string::npos);  // re-uploaded
        QCOMPARE(transmittedImage(out).pixelColor(0, 0), QColor(Qt::blue));
        uint32_t colour = 0;
        QCOMPARE(placeholderCellCount(engine, &colour), 18);
        QCOMPARE(colour & 0xFFFFFFu, firstId);
        QVERIFY(out.find(std::string(kPlaceholderUtf8, 4)) == std::string::npos);
    }

    void largeLiveFramesKeepPlaceholderCells() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(240, 72));
        img->setWidth(230);
        img->setHeight(60);
        QCOMPARE(img->renderSize(), QSize(2048, 1068));
        QImage pixels(img->renderSize(), QImage::Format_RGB32);
        pixels.fill(Qt::red);
        img->setImage(pixels);
        QTRY_VERIFY(engine->frameSequence() > 0);
        uint32_t firstColour = 0;
        QCOMPARE(placeholderCellCount(engine, &firstColour), 230 * 60);
        const auto firstFrame = engine->currentFrame();

        for (Qt::GlobalColor colour : {Qt::blue, Qt::green, Qt::red}) {
            backend->clearOutput();
            pixels.fill(colour);
            const auto before = engine->frameSequence();
            img->setImage(pixels);
            QTRY_VERIFY(engine->frameSequence() > before);
            const auto& out = backend->output();
            const QImage transmitted = transmittedImage(out);
            QCOMPARE(transmitted.size(), img->renderSize());
            QCOMPARE(transmitted.pixelColor(0, 0), QColor(colour));
            QCOMPARE(engine->currentFrame()->toAnsiString(), firstFrame->toAnsiString());
            QVERIFY(out.find(std::string(kPlaceholderUtf8, 4)) == std::string::npos);
            QVERIFY(out.find("\033_Ga=d") == std::string::npos);
            QVERIFY(out.find("\033_Ga=p,U=1,q=2,i=" + std::to_string(firstColour & 0xFFFFFFu)) != std::string::npos);
        }

        // A new output session must still upload the image and reconstruct
        // the placeholders, even though the pixels and id haven't changed.
        backend->clearOutput();
        const auto before = engine->frameSequence();
        engine->setTerminalOutputEnabled(false);
        engine->setTerminalOutputEnabled(true);
        QTRY_VERIFY(engine->frameSequence() > before);
        QCOMPARE(transmittedImage(backend->output()).pixelColor(0, 0), QColor(Qt::red));
        uint32_t afterReset = 0;
        QCOMPARE(placeholderCellCount(engine, &afterReset), 230 * 60);
        QCOMPARE(afterReset, firstColour);
    }

    void resizedLiveImageReplacesVirtualPlacement() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        auto* engine = terminal->engine();
        engine->setTestViewportSize(QSize(250, 72));

        // Ghostty retains placements across same-id image uploads. Model that
        // lifetime: unnamed placements accumulate, while a named one replaces
        // the geometry for its (image id, placement id) pair.
        QMap<QPair<quint32, quint32>, QSize> placements;
        quint32 unnamedId = 0;
        quint32 imageId = 0;
        quint32 placementId = 0;
        const QRegularExpression command(
            "\\033_Ga=p,U=1,q=2,i=(\\d+)(?:,p=(\\d+))?,c=(\\d+),r=(\\d+)");
        for (QSize size : {QSize(6, 3), QSize(240, 60), QSize(240, 60), QSize(80, 24)}) {
            backend->clearOutput();
            const auto before = engine->frameSequence();
            img->setWidth(size.width());
            img->setHeight(size.height());
            QImage pixels(img->renderSize(), QImage::Format_RGB32);
            pixels.fill(Qt::green);
            img->setImage(pixels);
            QTRY_VERIFY(engine->frameSequence() > before);

            uint32_t foreground = 0;
            QCOMPARE(placeholderCellCount(engine, &foreground), size.width() * size.height());
            const auto match = command.match(QString::fromStdString(backend->output()));
            QVERIFY(match.hasMatch());
            const quint32 id = match.captured(1).toUInt();
            const quint32 pid = match.captured(2).toUInt();
            const QSize placed(match.captured(3).toInt(), match.captured(4).toInt());
            placements.insert(qMakePair(id, pid ? pid : ++unnamedId), placed);
            QVERIFY2(pid != 0, "Unnamed placements retain obsolete image geometry in Ghostty");
            if (imageId == 0) {
                imageId = id;
                placementId = pid;
            }
            QCOMPARE(id, imageId);
            QCOMPARE(pid, placementId);
            QCOMPARE(foreground & 0xFFFFFFu, imageId);
            QCOMPARE(placements.size(), 1);
            QCOMPARE(placements.first(), size);
        }
    }

    void pendingReplacementRetention_data() {
        QTest::addColumn<int>("protocol");
        QTest::addColumn<bool>("retain");
        QTest::newRow("kitty-retain") << 1 << true;
        QTest::newRow("kitty-clear") << 1 << false;
        QTest::newRow("sixel-retain") << 2 << true;
        QTest::newRow("sixel-clear") << 2 << false;
        QTest::newRow("half-block-retain") << 0 << true;
        QTest::newRow("half-block-clear") << 0 << false;
    }

    void pendingReplacementRetention() {
        QFETCH(int, protocol);
        QFETCH(bool, retain);
        const bool kitty = protocol == 1;
        const bool sixel = protocol == 2;
        QQmlEngine qml;
        auto* provider = new PendingProvider;
        ImageProvider::add(&qml, QStringLiteral("pending"), provider);
        std::unique_ptr<QObject> holder;
        TerminalCapabilities caps;
        caps.colorDepth = ColorDepth::TrueColor;
        Terminal* terminal = buildTerminal(qml, holder, kitty ? kittyCaps() : sixel ? sixelCaps() : caps);
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        QVERIFY(!img->retainWhileLoading());
        img->setRetainWhileLoading(retain);
        auto* engine = terminal->engine();
        img->setSource(QUrl::fromLocalFile(redPng));
        QTRY_VERIFY(engine->frameSequence() > 0);
        const auto original = engine->currentFrame();
        QVERIFY(original);
        if (kitty) {
            QCOMPARE(placeholderCellCount(engine), 18);
            QCOMPARE(transmittedImage(backend->output()).pixelColor(0, 0), QColor(Qt::red));
        } else if (sixel) {
            QVERIFY(backend->output().find("\033P0;1q") != std::string::npos);
        } else {
            QCOMPARE(original->cellAt(0, 0).fgColor, uint32_t(qRgb(255, 0, 0)));
        }

        // A slow response spans several commits; newer requests can also
        // supersede it. Each commit either retains the image or clears it.
        for (int request = 0; request < 3; ++request) {
            backend->clearOutput();
            img->setSource(QUrl(QStringLiteral("image://pending/%1").arg(request)));
            QCOMPARE(img->status(), Image::Status::Loading);
            const auto before = engine->frameSequence();
            engine->requestRender();
            QTRY_VERIFY(engine->frameSequence() > before);
            if (retain) {
                QCOMPARE(engine->currentFrame()->toAnsiString(), original->toAnsiString());
                // Retaining cells is insufficient if the upload was deleted.
                QVERIFY(backend->output().find("\033_G") == std::string::npos);
                QVERIFY(backend->output().find("\033[0m\0337") == std::string::npos);
            } else {
                QVERIFY(img->image().isNull());
                QCOMPARE(placeholderCellCount(engine), 0);
                QCOMPARE(engine->currentFrame()->cellAt(0, 0).character().toByteArray(), QByteArray(" "));
                if (kitty && request == 0)
                    QVERIFY(backend->output().find("\033_Ga=d,d=I") != std::string::npos);
                if (sixel && request == 0)
                    QVERIFY(backend->output().find("\033[0m\0337\033[1;1H      ") != std::string::npos);
            }
        }

        backend->clearOutput();
        auto before = engine->frameSequence();
        provider->pending->complete(QImage(bluePng));
        QTRY_VERIFY(engine->frameSequence() > before);
        QCOMPARE(img->status(), Image::Status::Ready);
        if (kitty) {
            QCOMPARE(placeholderCellCount(engine), 18);
            QCOMPARE(transmittedImage(backend->output()).pixelColor(0, 0), QColor(Qt::blue));
            if (retain) {
                QVERIFY(backend->output().find("\033_Ga=d,d=I") == std::string::npos);
                QCOMPARE(engine->currentFrame()->cellAt(0, 0).fgColor, original->cellAt(0, 0).fgColor);
            }
        } else if (sixel) {
            QVERIFY(backend->output().find("\033P0;1q") != std::string::npos);
        } else {
            QCOMPARE(engine->currentFrame()->cellAt(0, 0).fgColor, uint32_t(qRgb(0, 0, 255)));
        }

        // Clearing or failing a pending source still removes the old image.
        for (bool clear : {true, false}) {
            img->setSource(QUrl::fromLocalFile(redPng));
            before = engine->frameSequence();
            engine->requestRender();
            QTRY_VERIFY(engine->frameSequence() > before);
            backend->clearOutput();
            img->setSource(QUrl(QStringLiteral("image://pending/final")));
            before = engine->frameSequence();
            if (clear) img->setSource(QUrl());
            else provider->pending->complete(QImage());
            QTRY_VERIFY(engine->frameSequence() > before);
            QCOMPARE(img->status(), clear ? Image::Status::Null : Image::Status::Error);
            QCOMPARE(placeholderCellCount(engine), 0);
            QCOMPARE(engine->currentFrame()->cellAt(0, 0).character().toByteArray(), QByteArray(" "));
            if (kitty)
                QVERIFY(backend->output().find("\033_Ga=d,d=I") != std::string::npos);
            if (sixel)
                QVERIFY(backend->output().find("\033[0m\0337\033[1;1H      ") != std::string::npos);
        }

        if (retain) {
            // Changing the policy mid-request discards the retained image,
            // without restarting the request or resurrecting discarded pixels.
            img->setSource(QUrl::fromLocalFile(redPng));
            before = engine->frameSequence();
            engine->requestRender();
            QTRY_VERIFY(engine->frameSequence() > before);
            img->setSource(QUrl(QStringLiteral("image://pending/toggle")));
            const auto pending = provider->pending;
            QSignalSpy changed(img, &Image::retainWhileLoadingChanged);
            before = engine->frameSequence();
            img->setRetainWhileLoading(false);
            img->setRetainWhileLoading(false);
            QCOMPARE(changed.count(), 1);
            QTRY_VERIFY(engine->frameSequence() > before);
            QCOMPARE(img->status(), Image::Status::Loading);
            QCOMPARE(engine->currentFrame()->cellAt(0, 0).character().toByteArray(), QByteArray(" "));
            QCOMPARE(placeholderCellCount(engine), 0);
            img->setRetainWhileLoading(true);
            QCOMPARE(changed.count(), 2);
            QVERIFY(img->image().isNull());
            QCOMPARE(provider->pending, pending);
            before = engine->frameSequence();
            pending->complete(QImage(bluePng));
            QTRY_VERIFY(engine->frameSequence() > before);
            QCOMPARE(img->status(), Image::Status::Ready);
            if (kitty) QCOMPARE(placeholderCellCount(engine), 18);
            else if (!sixel)
                QCOMPARE(engine->currentFrame()->cellAt(0, 0).fgColor, uint32_t(qRgb(0, 0, 255)));
        }
    }

    void engineStopFreesAllImages() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, kittyCaps());
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(placeholderCellCount(engine) > 0);
        backend->clearOutput();
        engine->stop();
        QVERIFY(backend->output().find("\033_Ga=d,d=A") != std::string::npos);
    }

    void autoFallsBackToHalfBlock() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, TerminalCapabilities{});
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(engine->frameSequence() > 0);
        QTest::qWait(30);
        QVERIFY(backend->output().find("\033_G") == std::string::npos);  // no graphics
        QCOMPARE(placeholderCellCount(engine), 0);
        // The half-block glyph is present instead.
        const auto frame = engine->currentFrame();
        bool halfBlock = false;
        for (int y = 0; y < frame->height() && !halfBlock; ++y)
            for (int x = 0; x < frame->width(); ++x)
                if (frame->cellAt(x, y).character().toByteArray() == QByteArray("\xE2\x96\x80")) { halfBlock = true; break; }
        QVERIFY(halfBlock);
    }

    void sixelEmittedAfterCellsOnlyWhenChanged() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, sixelCaps());
        QVERIFY(terminal);
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));
        auto* engine = terminal->engine();
        QTRY_VERIFY(backend->output().find("\033P0;1q") != std::string::npos);

        const std::string& out = backend->output();
        QVERIFY(out.find("\033_G") == std::string::npos);          // not kitty
        QCOMPARE(placeholderCellCount(engine), 0);
        // Saved cursor, absolute move, then the sixel, then restore.
        const auto save = out.find("\0337\033[");
        const auto dcs = out.find("\033P0;1q");
        const auto restore = out.rfind("\0338");
        QVERIFY(save != std::string::npos);
        QVERIFY(save < dcs);
        QVERIFY(dcs < restore);
        // The image rectangle is cleared with blanks before the sixel, so
        // frames do not accumulate under each other (sixel leaves unset pixels).
        QVERIFY(out.find(std::string(6, ' '), save) < dcs);   // Image is 6 cells wide
        // The sixel lands inside the synchronized-output bracket, after cells.
        QVERIFY(out.find("\033[?2026h") < dcs);
        QVERIFY(dcs < out.rfind("\033[?2026l"));

        // An unchanged frame re-sends no sixel.
        backend->clearOutput();
        const auto seq = engine->frameSequence();
        engine->requestRender();
        QTRY_VERIFY(engine->frameSequence() > seq);
        QVERIFY(backend->output().find("\033P") == std::string::npos);

        // A source change re-emits.
        backend->clearOutput();
        img->setSource(QUrl::fromLocalFile(bluePng));
        QTRY_VERIFY(backend->output().find("\033P0;1q") != std::string::npos);
    }

    void autoPicksSixelWhenOnlySixel() {
        QQmlEngine qml;
        std::unique_ptr<QObject> holder;
        Terminal* terminal = buildTerminal(qml, holder, sixelCaps());
        auto* img = terminal->findChild<Image*>("img");
        img->setSource(QUrl::fromLocalFile(redPng));  // Image is in Auto mode
        auto* engine = terminal->engine();
        QTRY_VERIFY(backend->output().find("\033P0;1q") != std::string::npos);
        QVERIFY(backend->output().find("\033_G") == std::string::npos);
        QCOMPARE(placeholderCellCount(engine), 0);
    }
};

QTEST_MAIN(TestKittyGraphics)
#include "test_kitty_graphics.moc"
