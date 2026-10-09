#include <QtTest/QtTest>
#include <Qtui/Image/Image.h>
#include <Qtui/Image/ImageProvider.h>
#include <QBuffer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <memory>
#include <Qtui/Core/primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <QTemporaryFile>
#include <Qtui/Core/primitives/Color.h>

class TestImage : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testLoadValidImage();
    void testLoadInvalidImage();
    void testRenderHalfBlock();
    void testAutoFitsLikeHalfBlock();
    void testSetImageFromMemory();
    void testImageProvider();
    void testAsyncImageProvider();
    void testRenderASCII();
    void testRenderBraille();
    void testFillModeScaling();
    void testQrcResourceUrl();
    void testRemoteUrl();
    void testTransparencyOverKnownBackground();
    void testTransparencyOverUnknownBackground();
    void testTransparencyAsciiAndBraille();

private:
    // Writes `pixels` (row-major, `width` per row) to a temporary PNG, which
    // keeps the alpha channel, and points `img` at it. Returns the file name
    // so the caller can remove it.
    QString loadPixels(qtui::Image& img, int width, const QList<QRgb>& pixels);
};

QString TestImage::loadPixels(qtui::Image& img, int width, const QList<QRgb>& pixels) {
    const int height = pixels.size() / width;
    QImage testImg(width, height, QImage::Format_ARGB32);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            testImg.setPixel(x, y, pixels[y * width + x]);

    QTemporaryFile tempFile;
    if (!tempFile.open()) return QString();
    const QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    if (!testImg.save(fileName, "PNG")) return QString();

    img.setSmooth(false);
    img.setSource(QUrl::fromLocalFile(fileName));
    return fileName;
}

// Pre-paints a cell the way a Rectangle or the root Terminal would, so the
// tests can tell an untouched cell from a repainted one.
static void primeCell(qtui::Cell& cell, uint32_t bgColor) {
    cell.setCharacter("X");
    cell.fgColor = 0xFFFFFFFFu;
    cell.bgColor = bgColor;
}

void TestImage::testInitialState() {
    qtui::Image img;
    QCOMPARE(img.status(), qtui::Image::Status::Null);
    QCOMPARE(img.fillMode(), qtui::Image::FillMode::Stretch);
    QCOMPARE(img.renderMode(), qtui::Image::RenderMode::Auto);  // resolves to HalfBlock without a kitty terminal
    QCOMPARE(img.smooth(), true);
    QCOMPARE(img.source().isEmpty(), true);
}

void TestImage::testLoadValidImage() {
    // Generate a 4x4 test PNG: top row red, bottom row blue
    QImage testImg(4, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::blue);
    for (int x = 0; x < 4; ++x) {
        testImg.setPixelColor(x, 0, Qt::red);
        testImg.setPixelColor(x, 1, Qt::red);
    }

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();

    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));

    QCOMPARE(img.status(), qtui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(4, 4));
    QCOMPARE(img.implicitWidth(), 4);
    QCOMPARE(img.implicitHeight(), 2); // 4 subpixels / 2 = 2 rows

    QFile::remove(fileName);
}

void TestImage::testLoadInvalidImage() {
    qtui::Image img;
    img.setSource(QUrl::fromLocalFile("/non_existent_path_12345.png"));
    QCOMPARE(img.status(), qtui::Image::Status::Error);
}

void TestImage::testRenderHalfBlock() {
    // Generate 2x2 test image: Top pixel Red (0xFFFF0000), Bottom pixel Blue (0xFF0000FF)
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.setPixelColor(0, 0, QColor(255, 0, 0));
    testImg.setPixelColor(1, 0, QColor(255, 0, 0));
    testImg.setPixelColor(0, 1, QColor(0, 0, 255));
    testImg.setPixelColor(1, 1, QColor(0, 0, 255));

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1); // 1 cell height = 2 subpixel rows

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    QFile::remove(fileName);
}

// Auto resolves to HalfBlock on a terminal without kitty or sixel; the picture
// must then be fitted to two pixel rows per cell exactly as HalfBlock does,
// not to one row per cell (which drew it at half height).
void TestImage::testAutoFitsLikeHalfBlock() {
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.setPixelColor(0, 0, QColor(255, 0, 0));
    testImg.setPixelColor(1, 0, QColor(255, 0, 0));
    testImg.setPixelColor(0, 1, QColor(0, 0, 255));
    testImg.setPixelColor(1, 1, QColor(0, 0, 255));
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    auto paintWith = [&](qtui::Image::RenderMode mode) {
        qtui::Image img;
        img.setSource(QUrl::fromLocalFile(fileName));
        img.setRenderMode(mode);
        img.setFillMode(qtui::Image::FillMode::PreserveAspectFit);
        img.setWidth(8);
        img.setHeight(4);   // 8 pixel rows: the 2x2 picture fits as 8x8 pixels = 8 cells x 4 rows
        qtui::TextGridBuffer buffer;
        buffer.resize(8, 4);
        qtui::testing::paintItem(img, buffer, 0, 0);
        QString picture;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 8; ++x) {
                const qtui::Cell& cell = buffer.cellAt(x, y);
                picture += QString::fromUtf8(cell.character().toByteArray()) + QString::number(cell.fgColor, 16) + "/" + QString::number(cell.bgColor, 16) + " ";
            }
            picture += "\n";
        }
        return picture;
    };
    const QString halfBlock = paintWith(qtui::Image::RenderMode::HalfBlock);
    const QString automatic = paintWith(qtui::Image::RenderMode::Auto);
    QVERIFY2(halfBlock.count(QStringLiteral("\u2580")) == 32, qPrintable(halfBlock));   // every cell painted
    QCOMPARE(automatic, halfBlock);
    QFile::remove(fileName);
}

// Shape 1: a QImage assigned from C++ renders like a loaded file, clears the
// URL, and a null image clears the item.
void TestImage::testSetImageFromMemory() {
    QImage frame(2, 2, QImage::Format_RGB32);
    frame.setPixelColor(0, 0, QColor(255, 0, 0));
    frame.setPixelColor(1, 0, QColor(255, 0, 0));
    frame.setPixelColor(0, 1, QColor(0, 0, 255));
    frame.setPixelColor(1, 1, QColor(0, 0, 255));

    qtui::Image img;
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);
    QSignalSpy imageChanged(&img, &qtui::Image::imageChanged);
    QSignalSpy statusChanged(&img, &qtui::Image::statusChanged);
    QSignalSpy sizeChanged(&img, &qtui::Image::sourceSizeChanged);

    img.setImage(frame);
    QCOMPARE(img.status(), qtui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(2, 2));
    QCOMPARE(img.image().size(), QSize(2, 2));
    QVERIFY(img.source().isEmpty());
    QCOMPARE(imageChanged.count(), 1);
    QCOMPARE(statusChanged.count(), 1);
    QCOMPARE(sizeChanged.count(), 1);

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);
    qtui::testing::paintItem(img, buffer, 0, 0);
    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    // A second frame of the same size: repainted, no size notification.
    frame.fill(Qt::green);
    img.setImage(frame);
    QCOMPARE(imageChanged.count(), 2);
    QCOMPARE(sizeChanged.count(), 1);
    qtui::testing::paintItem(img, buffer, 0, 0);
    QCOMPARE(QColor::fromRgba(buffer.cellAt(0, 0).fgColor).green(), 255);

    // A null image clears the item.
    img.setImage(QImage());
    QCOMPARE(img.status(), qtui::Image::Status::Null);
    QVERIFY(img.image().isNull());
    QCOMPARE(sizeChanged.count(), 2);
}

namespace {
class SolidProvider : public qtui::ImageProvider {
public:
    QStringList ids;
    QSize lastRequested;
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        ids.append(id);
        lastRequested = requestedSize;
        if (!id.startsWith(QLatin1String("solid"))) return QImage();
        QImage image(4, 4, QImage::Format_RGB32);
        image.fill(id.contains(QLatin1String("blue")) ? Qt::blue : Qt::red);
        *size = image.size();
        return image;
    }
};
}

// Shape 2: an image:// URL resolves through the ImageProvider installed on
// the item's engine; the id carries path and query; unknown ids and unknown
// providers report Error without crashing.
void TestImage::testImageProvider() {
    QQmlEngine qml;
    auto* provider = new SolidProvider;
    qtui::ImageProvider::add(&qml, QStringLiteral("test"), provider);
    QCOMPARE(qtui::ImageProvider::find(&qml, QStringLiteral("test")), provider);
    QCOMPARE(provider->parent(), &qml);

    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qtui
import Qtui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 4; height: 2; renderMode: Image.HalfBlock
            source: "image://test/solid?frame=1" }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qtui::Terminal*>(object.get());
    QVERIFY(terminal);
    auto* picture = terminal->findChild<qtui::Image*>("picture");
    QVERIFY(picture);
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(4, 4));
    QCOMPARE(provider->ids, QStringList{"solid?frame=1"});

    auto* engine = terminal->engine();
    engine->setTestViewportSize(QSize(10, 4));
    QTRY_VERIFY(engine->frameSequence() > 0);
    const auto frame = engine->currentFrame();
    QVERIFY(frame);
    QCOMPARE(QColor::fromRgba(frame->cellAt(0, 0).fgColor).red(), 255);

    // A new URL asks the provider again; the picture follows.
    picture->setSource(QUrl(QStringLiteral("image://test/solid-blue?frame=2")));
    QCOMPARE(provider->ids.size(), 2);
    QCOMPARE(provider->ids.last(), QStringLiteral("solid-blue?frame=2"));
    QCOMPARE(provider->lastRequested, QSize(4, 4));      // 4 columns, 2 rows of half-blocks
    const quint64 before = engine->frameSequence();
    engine->requestRender();
    QTRY_VERIFY(engine->frameSequence() > before);
    QCOMPARE(QColor::fromRgba(engine->currentFrame()->cellAt(0, 0).fgColor).blue(), 255);

    // Unknown id: the provider returns null, the item reports Error.
    picture->setSource(QUrl(QStringLiteral("image://test/missing")));
    QCOMPARE(picture->status(), qtui::Image::Status::Error);
    // Unknown provider: Error as well, and a warning.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("no qtui::ImageProvider named \"nobody\""));
    picture->setSource(QUrl(QStringLiteral("image://nobody/x")));
    QCOMPARE(picture->status(), qtui::Image::Status::Error);

    // Replacing a provider deletes the previous one.
    QPointer<qtui::ImageProvider> old(provider);
    qtui::ImageProvider::add(&qml, QStringLiteral("test"), new SolidProvider);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!old);
    qtui::ImageProvider::remove(&qml, QStringLiteral("test"));
    QVERIFY(!qtui::ImageProvider::find(&qml, QStringLiteral("test")));
}

namespace {
// An asynchronous provider the test completes by hand, so timing is explicit.
class LaterResponse : public qtui::ImageResponse {
public:
    QImage img;
    bool cancelled = false;
    QImage image() const override { return img; }
    void cancel() override { cancelled = true; }
    void complete(const QImage& picture) { img = picture; emit finished(); }
};
class LaterProvider : public qtui::AsyncImageProvider {
public:
    QStringList ids;
    QList<QPointer<LaterResponse>> responses;
    qtui::ImageResponse* requestImageResponse(const QString& id, const QSize&) override {
        ids.append(id);
        if (id == QLatin1String("refuse")) return nullptr;
        auto* response = new LaterResponse;
        responses.append(response);
        return response;
    }
};
QImage solid(Qt::GlobalColor colour) {
    QImage image(4, 4, QImage::Format_RGB32);
    image.fill(colour);
    return image;
}
}

// The asynchronous shape: the item reports Loading and keeps its current
// picture until the response finishes; a superseded response is cancelled and
// ignored; a refused request or a null picture reports Error.
void TestImage::testAsyncImageProvider() {
    QQmlEngine qml;
    auto* provider = new LaterProvider;
    qtui::ImageProvider::add(&qml, QStringLiteral("later"), provider);
    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qtui
import Qtui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 4; height: 2; renderMode: Image.HalfBlock
            source: "image://later/first" }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qtui::Terminal*>(object.get());
    auto* picture = terminal->findChild<qtui::Image*>("picture");
    QVERIFY(picture);
    QCOMPARE(picture->status(), qtui::Image::Status::Loading);
    QCOMPARE(provider->ids, QStringList{"first"});
    QCOMPARE(provider->responses.size(), 1);

    provider->responses[0]->complete(solid(Qt::red));
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(4, 4));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!provider->responses[0]);            // the item deleted the finished response

    // A new request keeps the old picture on screen while it is pending.
    picture->setSource(QUrl(QStringLiteral("image://later/second")));
    QCOMPARE(picture->status(), qtui::Image::Status::Loading);
    QVERIFY(!picture->image().isNull());
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::red));

    // Superseding it cancels the pending response, whose late finish is ignored.
    picture->setSource(QUrl(QStringLiteral("image://later/third")));
    QCOMPARE(provider->responses.size(), 3);
    QVERIFY(provider->responses[1]->cancelled);
    provider->responses[1]->complete(solid(Qt::green));
    QCOMPARE(picture->status(), qtui::Image::Status::Loading);
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::red));
    provider->responses[2]->complete(solid(Qt::blue));
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::blue));

    // Refused at once, and a null picture, both report Error.
    picture->setSource(QUrl(QStringLiteral("image://later/refuse")));
    QCOMPARE(picture->status(), qtui::Image::Status::Error);
    picture->setSource(QUrl(QStringLiteral("image://later/fourth")));
    provider->responses.last()->complete(QImage());
    QCOMPARE(picture->status(), qtui::Image::Status::Error);

    // Assigning a picture directly drops an outstanding request.
    picture->setSource(QUrl(QStringLiteral("image://later/fifth")));
    QPointer<LaterResponse> pending = provider->responses.last();
    picture->setImage(solid(Qt::yellow));
    QVERIFY(pending->cancelled);
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
}

void TestImage::testRenderASCII() {
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.fill(Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::ASCII);
    img.setWidth(2);
    img.setHeight(2);

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("@"));

    QFile::remove(fileName);
}

void TestImage::testRenderBraille() {
    QImage testImg(2, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qtui::Image::RenderMode::Braille);
    img.setWidth(1);
    img.setHeight(1);

    qtui::TextGridBuffer buffer;
    buffer.resize(2, 2);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& cell0 = buffer.cellAt(0, 0);
    QVERIFY(!cell0.character().isEmpty());

    QFile::remove(fileName);
}

void TestImage::testFillModeScaling() {
    QImage testImg(100, 50, QImage::Format_ARGB32);
    testImg.fill(Qt::green);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qtui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setFillMode(qtui::Image::FillMode::PreserveAspectFit);
    img.setWidth(20);
    img.setHeight(10);

    qtui::TextGridBuffer buffer;
    buffer.resize(20, 10);
    qtui::testing::paintItem(img, buffer, 0, 0);

    QCOMPARE(img.status(), qtui::Image::Status::Ready);

    QFile::remove(fileName);
}

void TestImage::testQrcResourceUrl() {
    qtui::Image img;
    img.setSource(QUrl("qrc:/qt/qml/ImageDemo/demo_image.png"));
    QVERIFY(img.status() == qtui::Image::Status::Error || img.status() == qtui::Image::Status::Ready);
}

// The transparency rule: zero alpha draws nothing, partial alpha composites
// source-over onto the background the cell holds, stored opaque. Each column of
// the 4x2 image is one half-block cell: top and bottom pixel per cell.
void TestImage::testTransparencyOverKnownBackground() {
    constexpr QRgb clear = 0x00000000u;
    constexpr QRgb halfRed = qRgba(255, 0, 0, 128);
    constexpr QRgb green = qRgba(0, 255, 0, 255);
    constexpr QRgb halfWhite = qRgba(255, 255, 255, 128);
    constexpr uint32_t blue = 0xFF0000FFu;

    qtui::Image img;
    const QString fileName = loadPixels(img, 4, {
        halfRed, clear, clear, halfWhite,   // top row
        clear,   green, clear, clear,       // bottom row
    });
    QVERIFY(!fileName.isEmpty());
    QCOMPARE(img.status(), qtui::Image::Status::Ready);
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(4);
    img.setHeight(1);

    qtui::TextGridBuffer buffer;
    buffer.resize(4, 1);
    for (int x = 0; x < 3; ++x)
        primeCell(buffer.cellAt(x, 0), blue);
    // Column 3 sits on an ANSI palette background: index 0, xterm black.
    primeCell(buffer.cellAt(3, 0), qtui::Color::fromAnsiIndex(0).argb32());

    qtui::testing::paintItem(img, buffer, 0, 0);

    // Half-transparent red over blue: the upper half shows the blend, the
    // lower half stays the blue that was there.
    const qtui::Cell& blended = buffer.cellAt(0, 0);
    QCOMPARE(blended.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(blended.fgColor, uint32_t(qRgb(128, 0, 127)));
    QCOMPARE(blended.bgColor, blue);

    // Only the lower pixel is painted: the glyph covers the lower half so the
    // upper half is the background, not a stale foreground.
    const qtui::Cell& lower = buffer.cellAt(1, 0);
    QCOMPARE(lower.character().toByteArray(), QByteArray("\xE2\x96\x84")); // ▄
    QCOMPARE(lower.fgColor, uint32_t(green));
    QCOMPARE(lower.bgColor, blue);

    // Two clear pixels leave the cell exactly as it was.
    const qtui::Cell& untouched = buffer.cellAt(2, 0);
    QCOMPARE(untouched.character().toByteArray(), QByteArray("X"));
    QCOMPARE(untouched.fgColor, 0xFFFFFFFFu);
    QCOMPARE(untouched.bgColor, blue);

    // A palette background blends through its xterm RGB: half white over black.
    const qtui::Cell& overAnsi = buffer.cellAt(3, 0);
    QCOMPARE(overAnsi.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(overAnsi.fgColor, uint32_t(qRgb(128, 128, 128)));
    QCOMPARE(overAnsi.bgColor, qtui::Color::fromAnsiIndex(0).argb32());

    QFile::remove(fileName);
}

// Over the terminal default background nothing is known to blend against, so
// pixels at least half opaque are drawn as they are and fainter ones are not.
void TestImage::testTransparencyOverUnknownBackground() {
    constexpr QRgb faintRed = qRgba(255, 0, 0, 100);
    constexpr QRgb mostlyGreen = qRgba(0, 255, 0, 200);

    qtui::Image img;
    const QString fileName = loadPixels(img, 2, {
        faintRed,    mostlyGreen,  // top row
        mostlyGreen, faintRed,     // bottom row
    });
    QVERIFY(!fileName.isEmpty());
    img.setRenderMode(qtui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);

    qtui::TextGridBuffer buffer;
    buffer.resize(2, 1);
    primeCell(buffer.cellAt(0, 0), 0x00000000u);
    primeCell(buffer.cellAt(1, 0), 0x00000000u);

    qtui::testing::paintItem(img, buffer, 0, 0);

    const qtui::Cell& lowerOnly = buffer.cellAt(0, 0);
    QCOMPARE(lowerOnly.character().toByteArray(), QByteArray("\xE2\x96\x84")); // ▄
    QCOMPARE(lowerOnly.fgColor, 0xFF00FF00u);   // unblended, stored opaque
    QCOMPARE(lowerOnly.bgColor, 0x00000000u);   // the faint pixel drew nothing

    const qtui::Cell& upperOnly = buffer.cellAt(1, 0);
    QCOMPARE(upperOnly.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(upperOnly.fgColor, 0xFF00FF00u);
    QCOMPARE(upperOnly.bgColor, 0x00000000u);

    QFile::remove(fileName);
}

// ASCII and Braille pick their glyph from the composited colour, so a faint
// pixel over a dark background is a dim glyph, or no dot at all.
void TestImage::testTransparencyAsciiAndBraille() {
    constexpr QRgb halfWhite = qRgba(255, 255, 255, 128);
    constexpr QRgb faintWhite = qRgba(255, 255, 255, 40);
    constexpr uint32_t black = 0xFF000000u;

    {
        qtui::Image img;
        const QString fileName = loadPixels(img, 2, { halfWhite, 0x00000000u });
        QVERIFY(!fileName.isEmpty());
        img.setRenderMode(qtui::Image::RenderMode::ASCII);
        img.setWidth(2);
        img.setHeight(1);

        qtui::TextGridBuffer buffer;
        buffer.resize(2, 1);
        primeCell(buffer.cellAt(0, 0), black);
        primeCell(buffer.cellAt(1, 0), black);
        qtui::testing::paintItem(img, buffer, 0, 0);

        // Half white over black is mid grey: the middle of the ramp, in grey.
        QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("="));
        QCOMPARE(buffer.cellAt(0, 0).fgColor, uint32_t(qRgb(128, 128, 128)));
        QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray("X"));
        QFile::remove(fileName);
    }
    {
        // One Braille cell is 2x4 pixels: the left column is faint, the right
        // column is half white. Only the right column clears the dark cut-off.
        qtui::Image img;
        const QString fileName = loadPixels(img, 2, {
            faintWhite, halfWhite,
            faintWhite, halfWhite,
            faintWhite, halfWhite,
            faintWhite, halfWhite,
        });
        QVERIFY(!fileName.isEmpty());
        img.setRenderMode(qtui::Image::RenderMode::Braille);
        img.setWidth(1);
        img.setHeight(1);

        qtui::TextGridBuffer buffer;
        buffer.resize(1, 1);
        primeCell(buffer.cellAt(0, 0), black);
        qtui::testing::paintItem(img, buffer, 0, 0);

        // Right column dots only: U+28B8 (dots 4, 5, 6, 8).
        QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QString(QChar(0x28B8)).toUtf8());
        QCOMPARE(buffer.cellAt(0, 0).fgColor, uint32_t(qRgb(128, 128, 128)));
        QFile::remove(fileName);
    }
}

namespace {
// A one-shot HTTP server on a local port: 200 with the PNG for /picture.png,
// 404 for anything else. Counts the requests so the cache can be observed.
class PictureServer : public QObject {
public:
    int requests = 0;
    QByteArray png;
    QTcpServer server;
    PictureServer() {
        QImage image(6, 4, QImage::Format_RGB32);
        image.fill(Qt::magenta);
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        connect(&server, &QTcpServer::newConnection, this, [this] {
            QTcpSocket* socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket] {
                const QByteArray request = socket->readAll();
                if (!request.contains("\r\n\r\n")) return;
                ++requests;
                const bool found = request.startsWith("GET /picture.png ");
                const QByteArray body = found ? png : QByteArray("missing");
                socket->write("HTTP/1.1 " + QByteArray(found ? "200 OK" : "404 Not Found") + "\r\n"
                              "Content-Type: " + QByteArray(found ? "image/png" : "text/plain") + "\r\n"
                              "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                              "Connection: close\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        });
        server.listen(QHostAddress::LocalHost);
    }
    QString url(const char* path) const {
        return QStringLiteral("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(QLatin1String(path));
    }
};
}

// An http source is fetched asynchronously: Loading with progress while the
// reply is in flight, Ready with the decoded picture afterwards, Error on a
// 404. A second item with the same URL is served from the cache without a
// request, and a superseded download is ignored.
void TestImage::testRemoteUrl() {
    PictureServer server;
    QQmlEngine qml;
    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qtui
import Qtui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 6; height: 2; renderMode: Image.HalfBlock }
    Image { objectName: "second"; x: 0; y: 2; width: 6; height: 2; renderMode: Image.HalfBlock }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qtui::Terminal*>(object.get());
    auto* picture = terminal->findChild<qtui::Image*>("picture");
    auto* second = terminal->findChild<qtui::Image*>("second");
    QVERIFY(picture && second);

    QSignalSpy progress(picture, &qtui::Image::progressChanged);
    picture->setSource(QUrl(server.url("/picture.png")));
    QCOMPARE(picture->status(), qtui::Image::Status::Loading);
    QTRY_COMPARE(picture->status(), qtui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(6, 4));
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::magenta));
    QCOMPARE(picture->progress(), 1.0);
    QVERIFY(progress.count() >= 1);
    QCOMPARE(server.requests, 1);

    // Cached: the second item is Ready at once and the server is not asked again.
    second->setSource(QUrl(server.url("/picture.png")));
    QCOMPARE(second->status(), qtui::Image::Status::Ready);
    QCOMPARE(second->sourceSize(), QSize(6, 4));
    QCOMPARE(server.requests, 1);

    // A missing picture reports Error and clears the item.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Image: http://127\\.0\\.0\\.1:\\d+/missing\\.png: .*"));
    picture->setSource(QUrl(server.url("/missing.png")));
    QCOMPARE(picture->status(), qtui::Image::Status::Loading);
    QVERIFY(!picture->image().isNull());   // the old picture stays until the reply
    QTRY_COMPARE(picture->status(), qtui::Image::Status::Error);
    QVERIFY(picture->image().isNull());
    QCOMPARE(picture->progress(), 0.0);
    QCOMPARE(server.requests, 2);

    // Superseding a download drops it: the later source wins.
    picture->setSource(QUrl(server.url("/also-missing.png")));
    picture->setSource(QUrl(server.url("/picture.png")));   // cached: Ready at once
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
    QTest::qWait(50);   // the aborted reply must not change anything
    QCOMPARE(picture->status(), qtui::Image::Status::Ready);
}

QTEST_MAIN(TestImage)
#include "test_image.moc"
