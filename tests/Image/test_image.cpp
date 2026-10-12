#include <QtTest/QtTest>
#include <Qamui/Image/Image.h>
#include <Qamui/Image/ImageProvider.h>
#include <QBuffer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSignalSpy>
#include <Qamui/Core/items/Terminal.h>
#include <Qamui/Core/engine/TerminalEngine.h>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <memory>
#include <Qamui/Core/primitives/TextGridBuffer.h>
#include "../TestPaint.h"
#include <QTemporaryFile>
#include <Qamui/Core/primitives/Color.h>
#include <Qamui/Core/primitives/Subpixel.h>

class TestImage : public QObject {
    Q_OBJECT

private slots:
    void testInitialState();
    void testLoadValidImage();
    void testLoadInvalidImage();
    void testRenderHalfBlock();
    void testHalfBlockSampling_data();
    void testHalfBlockSampling();
    void testAutoFitsLikeHalfBlock();
    void testSetImageFromMemory();
    void testImageProvider();
    void testAsyncImageProvider();
    void testRenderASCII();
    void testRenderBraille();
    void testBrailleIndividualDots_data();
    void testBrailleIndividualDots();
    void testZeroMaskPreservesUnderlyingCell();
    void testFillModeScaling();
    void testSourceClipRect();
    void testSourceClipRectOutside();
    void testQrcResourceUrl();
    void testRemoteUrl_data();
    void testRemoteUrl();
    void testTransparencyOverKnownBackground();
    void testTransparencyOverUnknownBackground();
    void testTransparencyAsciiAndBraille();

private:
    // Writes `pixels` (row-major, `width` per row) to a temporary PNG, which
    // keeps the alpha channel, and points `img` at it. Returns the file name
    // so the caller can remove it.
    QString loadPixels(qamui::Image& img, int width, const QList<QRgb>& pixels);
};

QString TestImage::loadPixels(qamui::Image& img, int width, const QList<QRgb>& pixels) {
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
static void primeCell(qamui::Cell& cell, uint32_t bgColor) {
    cell.setCharacter("X");
    cell.fgColor = 0xFFFFFFFFu;
    cell.bgColor = bgColor;
}

void TestImage::testInitialState() {
    qamui::Image img;
    QCOMPARE(img.status(), qamui::Image::Status::Null);
    QCOMPARE(img.fillMode(), qamui::Image::FillMode::Stretch);
    QCOMPARE(img.renderMode(), qamui::Image::RenderMode::Auto);  // resolves to HalfBlock without a kitty terminal
    QCOMPARE(img.smooth(), false);
    QCOMPARE(img.source().isEmpty(), true);
    QCOMPARE(img.retainWhileLoading(), false);
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

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));

    QCOMPARE(img.status(), qamui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(4, 4));
    QCOMPARE(img.implicitWidth(), 4);
    QCOMPARE(img.implicitHeight(), 2); // 4 subpixels / 2 = 2 rows

    QFile::remove(fileName);
}

void TestImage::testLoadInvalidImage() {
    qamui::Image img;
    img.setSource(QUrl::fromLocalFile("/non_existent_path_12345.png"));
    QCOMPARE(img.status(), qamui::Image::Status::Error);
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

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1); // 1 cell height = 2 subpixel rows

    qamui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qamui::testing::paintItem(img, buffer, 0, 0);

    const qamui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    QFile::remove(fileName);
}

void TestImage::testHalfBlockSampling_data() {
    QTest::addColumn<QSize>("cells");
    QTest::addColumn<qamui::Image::RenderMode>("mode");
    QTest::newRow("half-block-smaller") << QSize(7, 2) << qamui::Image::RenderMode::HalfBlock;
    QTest::newRow("half-block-larger") << QSize(17, 5) << qamui::Image::RenderMode::HalfBlock;
    QTest::newRow("auto-smaller") << QSize(7, 2) << qamui::Image::RenderMode::Auto;
    QTest::newRow("auto-larger") << QSize(17, 5) << qamui::Image::RenderMode::Auto;
    QTest::newRow("kitty-fallback") << QSize(7, 2) << qamui::Image::RenderMode::Kitty;
    QTest::newRow("sixel-fallback") << QSize(7, 2) << qamui::Image::RenderMode::Sixel;
    QTest::newRow("native-size") << QSize(10, 3) << qamui::Image::RenderMode::HalfBlock;
}

void TestImage::testHalfBlockSampling() {
    QFETCH(QSize, cells);
    QFETCH(qamui::Image::RenderMode, mode);
    // An opaque black/white edge contains no intermediate shades to begin
    // with. Any grey in the rendered cells must come from resampling.
    QImage source(10, 6, QImage::Format_RGB32);
    for (int y = 0; y < source.height(); ++y)
        for (int x = 0; x < source.width(); ++x)
            source.setPixel(x, y, x < 5 ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
    qamui::Image img;
    img.setImage(source);
    img.setRenderMode(mode);
    img.setWidth(cells.width());
    img.setHeight(cells.height());
    auto render = [&] {
        qamui::TextGridBuffer frame;
        frame.resize(cells.width(), cells.height());
        qamui::testing::paintItem(img, frame, 0, 0);
        return frame;
    };
    auto blendedHalves = [](const qamui::TextGridBuffer& frame) {
        int count = 0;
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                const auto& cell = frame.cellAt(x, y);
                for (uint32_t colour : {cell.fgColor, cell.bgColor})
                    if (colour != qRgb(0, 0, 0) && colour != qRgb(255, 255, 255)) ++count;
            }
        }
        return count;
    };

    // The default must preserve the source palette at any output size.
    QVERIFY(!img.smooth());
    const auto sharp = render();
    QCOMPARE(blendedHalves(sharp), 0);
    QCOMPARE(sharp.cellAt(0, 0).fgColor, qRgb(0, 0, 0));
    QCOMPARE(sharp.cellAt(cells.width() - 1, 0).fgColor, qRgb(255, 255, 255));
    img.setSmooth(true);
    const auto filtered = render();
    const bool resized = QSize(cells.width(), cells.height() * 2) != source.size();
    QCOMPARE(blendedHalves(filtered) > 0, resized);
    img.setSmooth(false);
    QCOMPARE(render().toAnsiString(), sharp.toAnsiString()); // changing sampling invalidates the cache
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

    auto paintWith = [&](qamui::Image::RenderMode mode) {
        qamui::Image img;
        img.setSource(QUrl::fromLocalFile(fileName));
        img.setRenderMode(mode);
        img.setFillMode(qamui::Image::FillMode::PreserveAspectFit);
        img.setWidth(8);
        img.setHeight(4);   // 8 pixel rows: the 2x2 picture fits as 8x8 pixels = 8 cells x 4 rows
        qamui::TextGridBuffer buffer;
        buffer.resize(8, 4);
        qamui::testing::paintItem(img, buffer, 0, 0);
        QString picture;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 8; ++x) {
                const qamui::Cell& cell = buffer.cellAt(x, y);
                picture += QString::fromUtf8(cell.character().toByteArray()) + QString::number(cell.fgColor, 16) + "/" + QString::number(cell.bgColor, 16) + " ";
            }
            picture += "\n";
        }
        return picture;
    };
    const QString halfBlock = paintWith(qamui::Image::RenderMode::HalfBlock);
    const QString automatic = paintWith(qamui::Image::RenderMode::Auto);
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

    qamui::Image img;
    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);
    QSignalSpy imageChanged(&img, &qamui::Image::imageChanged);
    QSignalSpy statusChanged(&img, &qamui::Image::statusChanged);
    QSignalSpy sizeChanged(&img, &qamui::Image::sourceSizeChanged);

    img.setImage(frame);
    QCOMPARE(img.status(), qamui::Image::Status::Ready);
    QCOMPARE(img.sourceSize(), QSize(2, 2));
    QCOMPARE(img.image().size(), QSize(2, 2));
    QVERIFY(img.source().isEmpty());
    QCOMPARE(imageChanged.count(), 1);
    QCOMPARE(statusChanged.count(), 1);
    QCOMPARE(sizeChanged.count(), 1);

    qamui::TextGridBuffer buffer;
    buffer.resize(4, 4);
    qamui::testing::paintItem(img, buffer, 0, 0);
    const qamui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(QColor::fromRgba(cell0.fgColor).red(), 255);
    QCOMPARE(QColor::fromRgba(cell0.bgColor).blue(), 255);

    // A second frame of the same size: repainted, no size notification.
    frame.fill(Qt::green);
    img.setImage(frame);
    QCOMPARE(imageChanged.count(), 2);
    QCOMPARE(sizeChanged.count(), 1);
    qamui::testing::paintItem(img, buffer, 0, 0);
    QCOMPARE(QColor::fromRgba(buffer.cellAt(0, 0).fgColor).green(), 255);

    // A null image clears the item.
    img.setImage(QImage());
    QCOMPARE(img.status(), qamui::Image::Status::Null);
    QVERIFY(img.image().isNull());
    QCOMPARE(sizeChanged.count(), 2);
}

namespace {
class SolidProvider : public qamui::ImageProvider {
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
    qamui::ImageProvider::add(&qml, QStringLiteral("test"), provider);
    QCOMPARE(qamui::ImageProvider::find(&qml, QStringLiteral("test")), provider);
    QCOMPARE(provider->parent(), &qml);

    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qamui
import Qamui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 4; height: 2; renderMode: Image.HalfBlock
            source: "image://test/solid?frame=1" }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qamui::Terminal*>(object.get());
    QVERIFY(terminal);
    auto* picture = terminal->findChild<qamui::Image*>("picture");
    QVERIFY(picture);
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
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
    QCOMPARE(picture->status(), qamui::Image::Status::Error);
    // Unknown provider: Error as well, and a warning.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("no qamui::ImageProvider named \"nobody\""));
    picture->setSource(QUrl(QStringLiteral("image://nobody/x")));
    QCOMPARE(picture->status(), qamui::Image::Status::Error);

    // Replacing a provider deletes the previous one.
    QPointer<qamui::ImageProvider> old(provider);
    qamui::ImageProvider::add(&qml, QStringLiteral("test"), new SolidProvider);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!old);
    qamui::ImageProvider::remove(&qml, QStringLiteral("test"));
    QVERIFY(!qamui::ImageProvider::find(&qml, QStringLiteral("test")));
}

namespace {
// An asynchronous provider the test completes by hand, so timing is explicit.
class LaterResponse : public qamui::ImageResponse {
public:
    QImage img;
    bool cancelled = false;
    QImage image() const override { return img; }
    void cancel() override { cancelled = true; }
    void complete(const QImage& picture) { img = picture; emit finished(); }
};
class LaterProvider : public qamui::AsyncImageProvider {
public:
    QStringList ids;
    QList<QPointer<LaterResponse>> responses;
    qamui::ImageResponse* requestImageResponse(const QString& id, const QSize&) override {
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
    qamui::ImageProvider::add(&qml, QStringLiteral("later"), provider);
    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qamui
import Qamui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 4; height: 2; renderMode: Image.HalfBlock
            retainWhileLoading: true
            source: "image://later/first" }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qamui::Terminal*>(object.get());
    auto* picture = terminal->findChild<qamui::Image*>("picture");
    QVERIFY(picture);
    QCOMPARE(picture->status(), qamui::Image::Status::Loading);
    QCOMPARE(provider->ids, QStringList{"first"});
    QCOMPARE(provider->responses.size(), 1);

    provider->responses[0]->complete(solid(Qt::red));
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(4, 4));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(!provider->responses[0]);            // the item deleted the finished response

    // A new request keeps the old picture on screen while it is pending.
    picture->setSource(QUrl(QStringLiteral("image://later/second")));
    QCOMPARE(picture->status(), qamui::Image::Status::Loading);
    QVERIFY(!picture->image().isNull());
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::red));

    // Superseding it cancels the pending response, whose late finish is ignored.
    picture->setSource(QUrl(QStringLiteral("image://later/third")));
    QCOMPARE(provider->responses.size(), 3);
    QVERIFY(provider->responses[1]->cancelled);
    provider->responses[1]->complete(solid(Qt::green));
    QCOMPARE(picture->status(), qamui::Image::Status::Loading);
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::red));
    provider->responses[2]->complete(solid(Qt::blue));
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::blue));

    // Refused at once, and a null picture, both report Error.
    picture->setSource(QUrl(QStringLiteral("image://later/refuse")));
    QCOMPARE(picture->status(), qamui::Image::Status::Error);
    picture->setSource(QUrl(QStringLiteral("image://later/fourth")));
    provider->responses.last()->complete(QImage());
    QCOMPARE(picture->status(), qamui::Image::Status::Error);

    // Assigning a picture directly drops an outstanding request.
    picture->setSource(QUrl(QStringLiteral("image://later/fifth")));
    QPointer<LaterResponse> pending = provider->responses.last();
    picture->setImage(solid(Qt::yellow));
    QVERIFY(pending->cancelled);
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
}

void TestImage::testRenderASCII() {
    QImage testImg(2, 2, QImage::Format_ARGB32);
    testImg.fill(Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qamui::Image::RenderMode::ASCII);
    img.setWidth(2);
    img.setHeight(2);

    qamui::TextGridBuffer buffer;
    buffer.resize(4, 4);

    qamui::testing::paintItem(img, buffer, 0, 0);

    const qamui::Cell& cell0 = buffer.cellAt(0, 0);
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

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qamui::Image::RenderMode::Braille);
    img.setWidth(1);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(2, 2);

    qamui::testing::paintItem(img, buffer, 0, 0);

    const qamui::Cell& cell0 = buffer.cellAt(0, 0);
    QCOMPARE(cell0.character().toByteArray(), qamui::braille::utf8(0xFF).toByteArray());

    QFile::remove(fileName);
}

void TestImage::testBrailleIndividualDots_data() {
    QTest::addColumn<int>("dotX");
    QTest::addColumn<int>("dotY");
    for (int y = 0; y < 4; ++y) {
        for (int x = 0; x < 2; ++x) {
            QTest::newRow(qPrintable(QStringLiteral("dot_%1_%2").arg(x).arg(y))) << x << y;
        }
    }
}

void TestImage::testBrailleIndividualDots() {
    QFETCH(int, dotX);
    QFETCH(int, dotY);

    QImage testImg(2, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::black);
    testImg.setPixelColor(dotX, dotY, Qt::white);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qamui::Image img;
    img.setSmooth(false);
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qamui::Image::RenderMode::Braille);
    img.setWidth(1);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(1, 1);
    qamui::testing::paintItem(img, buffer, 0, 0);

    const qamui::Cell& cell0 = buffer.cellAt(0, 0);
    const uint8_t expectedMask = qamui::braille::dotMask(dotX, dotY);
    QCOMPARE(cell0.character().toByteArray(), qamui::braille::utf8(expectedMask).toByteArray());

    QFile::remove(fileName);
}

void TestImage::testZeroMaskPreservesUnderlyingCell() {
    // A fully transparent image should produce no Braille dots and leave the
    // existing cell character and colours untouched.
    QImage testImg(2, 4, QImage::Format_ARGB32);
    testImg.fill(Qt::transparent);

    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString fileName = tempFile.fileName() + ".png";
    tempFile.close();
    QVERIFY(testImg.save(fileName, "PNG"));

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setRenderMode(qamui::Image::RenderMode::Braille);
    img.setWidth(1);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(1, 1);
    qamui::Cell& cell = buffer.cellAt(0, 0);
    cell.setCharacter("Z");
    cell.fgColor = 0xFF112233;
    cell.bgColor = 0xFF445566;

    qamui::testing::paintItem(img, buffer, 0, 0);

    QCOMPARE(cell.character().toByteArray(), QByteArray("Z"));
    QCOMPARE(cell.fgColor, 0xFF112233u);
    QCOMPARE(cell.bgColor, 0xFF445566u);

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

    qamui::Image img;
    img.setSource(QUrl::fromLocalFile(fileName));
    img.setFillMode(qamui::Image::FillMode::PreserveAspectFit);
    img.setWidth(20);
    img.setHeight(10);

    qamui::TextGridBuffer buffer;
    buffer.resize(20, 10);
    qamui::testing::paintItem(img, buffer, 0, 0);

    QCOMPARE(img.status(), qamui::Image::Status::Ready);

    QFile::remove(fileName);
}

void TestImage::testSourceClipRect() {
    // 4x4 quadrants: top-left red, top-right green, bottom-left blue,
    // bottom-right yellow, each 2x2.
    QImage sheet(4, 4, QImage::Format_ARGB32);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            sheet.setPixel(x, y, y < 2 ? (x < 2 ? qRgb(255, 0, 0) : qRgb(0, 255, 0))
                                       : (x < 2 ? qRgb(0, 0, 255) : qRgb(255, 255, 0)));

    qamui::Image img;
    img.setImage(sheet);
    QCOMPARE(img.sourceClipRect(), QRect());
    QCOMPARE(img.implicitWidth(), 4);

    QSignalSpy clipSpy(&img, &qamui::Image::sourceClipRectChanged);
    img.setSourceClipRect(QRect(2, 2, 2, 2));   // the yellow quadrant
    QCOMPARE(clipSpy.count(), 1);
    // The implicit size follows the clip; sourceSize keeps the full picture.
    QCOMPARE(img.implicitWidth(), 2);
    QCOMPARE(img.implicitHeight(), 1);
    QCOMPARE(img.sourceSize(), QSize(4, 4));

    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);
    qamui::TextGridBuffer buffer;
    buffer.resize(4, 4);
    qamui::testing::paintItem(img, buffer, 0, 0);
    for (int x = 0; x < 2; ++x) {
        const qamui::Cell& cell = buffer.cellAt(x, 0);
        QCOMPARE(cell.fgColor, 0xFFFFFF00u);   // both halves yellow
        QCOMPARE(cell.bgColor, 0xFFFFFF00u);
    }

    // A clip reaching outside the picture is cut to it: the same quadrant.
    img.setSourceClipRect(QRect(2, 2, 4, 4));
    qamui::TextGridBuffer buffer2;
    buffer2.resize(4, 4);
    qamui::testing::paintItem(img, buffer2, 0, 0);
    QCOMPARE(buffer2.cellAt(0, 0).fgColor, 0xFFFFFF00u);

    img.resetSourceClipRect();
    QCOMPARE(img.sourceClipRect(), QRect());
    QCOMPARE(img.implicitWidth(), 4);
}

void TestImage::testSourceClipRectOutside() {
    QImage sheet(4, 4, QImage::Format_ARGB32);
    sheet.fill(QColor(255, 0, 0));

    qamui::Image img;
    img.setImage(sheet);
    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setSourceClipRect(QRect(10, 10, 2, 2));   // entirely outside
    img.setWidth(2);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(4, 4);
    primeCell(buffer.cellAt(0, 0), 0xFF123456u);
    qamui::testing::paintItem(img, buffer, 0, 0);

    // Nothing to show: the cell keeps its glyph and colours.
    const qamui::Cell& cell = buffer.cellAt(0, 0);
    QCOMPARE(cell.character().toByteArray(), QByteArray("X"));
    QCOMPARE(cell.bgColor, 0xFF123456u);
}

void TestImage::testQrcResourceUrl() {
    qamui::Image img;
    img.setSource(QUrl("qrc:/qt/qml/ImageDemo/demo_image.png"));
    QVERIFY(img.status() == qamui::Image::Status::Error || img.status() == qamui::Image::Status::Ready);
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

    qamui::Image img;
    const QString fileName = loadPixels(img, 4, {
        halfRed, clear, clear, halfWhite,   // top row
        clear,   green, clear, clear,       // bottom row
    });
    QVERIFY(!fileName.isEmpty());
    QCOMPARE(img.status(), qamui::Image::Status::Ready);
    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setWidth(4);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(4, 1);
    for (int x = 0; x < 3; ++x)
        primeCell(buffer.cellAt(x, 0), blue);
    // Column 3 sits on an ANSI palette background: index 0, xterm black.
    primeCell(buffer.cellAt(3, 0), qamui::Color::fromAnsiIndex(0).argb32());

    qamui::testing::paintItem(img, buffer, 0, 0);

    // Half-transparent red over blue: the upper half shows the blend, the
    // lower half stays the blue that was there.
    const qamui::Cell& blended = buffer.cellAt(0, 0);
    QCOMPARE(blended.character().toByteArray(), QByteArray("\xE2\x96\x80")); // ▀
    QCOMPARE(blended.fgColor, uint32_t(qRgb(128, 0, 127)));
    QCOMPARE(blended.bgColor, blue);

    // Only the lower pixel is painted: the glyph covers the lower half so the
    // upper half is the background, not a stale foreground.
    const qamui::Cell& lower = buffer.cellAt(1, 0);
    QCOMPARE(lower.character().toByteArray(), QByteArray("\xE2\x96\x84")); // ▄
    QCOMPARE(lower.fgColor, uint32_t(green));
    QCOMPARE(lower.bgColor, blue);

    // Two clear pixels leave the cell exactly as it was.
    const qamui::Cell& untouched = buffer.cellAt(2, 0);
    QCOMPARE(untouched.character().toByteArray(), QByteArray("X"));
    QCOMPARE(untouched.fgColor, 0xFFFFFFFFu);
    QCOMPARE(untouched.bgColor, blue);

    // A palette background blends through its xterm RGB: half white over black.
    const qamui::Cell& overAnsi = buffer.cellAt(3, 0);
    QCOMPARE(overAnsi.character().toByteArray(), QByteArray("\xE2\x96\x80"));
    QCOMPARE(overAnsi.fgColor, uint32_t(qRgb(128, 128, 128)));
    QCOMPARE(overAnsi.bgColor, qamui::Color::fromAnsiIndex(0).argb32());

    QFile::remove(fileName);
}

// Over the terminal default background nothing is known to blend against, so
// pixels at least half opaque are drawn as they are and fainter ones are not.
void TestImage::testTransparencyOverUnknownBackground() {
    constexpr QRgb faintRed = qRgba(255, 0, 0, 100);
    constexpr QRgb mostlyGreen = qRgba(0, 255, 0, 200);

    qamui::Image img;
    const QString fileName = loadPixels(img, 2, {
        faintRed,    mostlyGreen,  // top row
        mostlyGreen, faintRed,     // bottom row
    });
    QVERIFY(!fileName.isEmpty());
    img.setRenderMode(qamui::Image::RenderMode::HalfBlock);
    img.setWidth(2);
    img.setHeight(1);

    qamui::TextGridBuffer buffer;
    buffer.resize(2, 1);
    primeCell(buffer.cellAt(0, 0), 0x00000000u);
    primeCell(buffer.cellAt(1, 0), 0x00000000u);

    qamui::testing::paintItem(img, buffer, 0, 0);

    const qamui::Cell& lowerOnly = buffer.cellAt(0, 0);
    QCOMPARE(lowerOnly.character().toByteArray(), QByteArray("\xE2\x96\x84")); // ▄
    QCOMPARE(lowerOnly.fgColor, 0xFF00FF00u);   // unblended, stored opaque
    QCOMPARE(lowerOnly.bgColor, 0x00000000u);   // the faint pixel drew nothing

    const qamui::Cell& upperOnly = buffer.cellAt(1, 0);
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
        qamui::Image img;
        const QString fileName = loadPixels(img, 2, { halfWhite, 0x00000000u });
        QVERIFY(!fileName.isEmpty());
        img.setRenderMode(qamui::Image::RenderMode::ASCII);
        img.setWidth(2);
        img.setHeight(1);

        qamui::TextGridBuffer buffer;
        buffer.resize(2, 1);
        primeCell(buffer.cellAt(0, 0), black);
        primeCell(buffer.cellAt(1, 0), black);
        qamui::testing::paintItem(img, buffer, 0, 0);

        // Half white over black is mid grey: the middle of the ramp, in grey.
        QCOMPARE(buffer.cellAt(0, 0).character().toByteArray(), QByteArray("="));
        QCOMPARE(buffer.cellAt(0, 0).fgColor, uint32_t(qRgb(128, 128, 128)));
        QCOMPARE(buffer.cellAt(1, 0).character().toByteArray(), QByteArray("X"));
        QFile::remove(fileName);
    }
    {
        // One Braille cell is 2x4 pixels: the left column is faint, the right
        // column is half white. Only the right column clears the dark cut-off.
        qamui::Image img;
        const QString fileName = loadPixels(img, 2, {
            faintWhite, halfWhite,
            faintWhite, halfWhite,
            faintWhite, halfWhite,
            faintWhite, halfWhite,
        });
        QVERIFY(!fileName.isEmpty());
        img.setRenderMode(qamui::Image::RenderMode::Braille);
        img.setWidth(1);
        img.setHeight(1);

        qamui::TextGridBuffer buffer;
        buffer.resize(1, 1);
        primeCell(buffer.cellAt(0, 0), black);
        qamui::testing::paintItem(img, buffer, 0, 0);

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
void TestImage::testRemoteUrl_data() {
    QTest::addColumn<bool>("retain");
    QTest::newRow("clear-while-loading") << false;
    QTest::newRow("retain-while-loading") << true;
}

void TestImage::testRemoteUrl() {
    QFETCH(bool, retain);
    PictureServer server;
    QQmlEngine qml;
    QQmlComponent component(&qml);
    component.setData(R"(
import QtQml
import Qamui
import Qamui.Image
Terminal {
    Image { objectName: "picture"; x: 0; y: 0; width: 6; height: 2; renderMode: Image.HalfBlock }
    Image { objectName: "second"; x: 0; y: 2; width: 6; height: 2; renderMode: Image.HalfBlock }
}
)", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto* terminal = qobject_cast<qamui::Terminal*>(object.get());
    auto* picture = terminal->findChild<qamui::Image*>("picture");
    auto* second = terminal->findChild<qamui::Image*>("second");
    QVERIFY(picture && second);
    picture->setRetainWhileLoading(retain);

    QSignalSpy progress(picture, &qamui::Image::progressChanged);
    picture->setSource(QUrl(server.url("/picture.png")));
    QCOMPARE(picture->status(), qamui::Image::Status::Loading);
    QTRY_COMPARE(picture->status(), qamui::Image::Status::Ready);
    QCOMPARE(picture->sourceSize(), QSize(6, 4));
    QCOMPARE(QColor(picture->image().pixel(0, 0)), QColor(Qt::magenta));
    QCOMPARE(picture->progress(), 1.0);
    QVERIFY(progress.count() >= 1);
    QCOMPARE(server.requests, 1);

    // Cached: the second item is Ready at once and the server is not asked again.
    second->setSource(QUrl(server.url("/picture.png")));
    QCOMPARE(second->status(), qamui::Image::Status::Ready);
    QCOMPARE(second->sourceSize(), QSize(6, 4));
    QCOMPARE(server.requests, 1);

    // A missing picture reports Error and clears the item.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Image: http://127\\.0\\.0\\.1:\\d+/missing\\.png: .*"));
    picture->setSource(QUrl(server.url("/missing.png")));
    QCOMPARE(picture->status(), qamui::Image::Status::Loading);
    QCOMPARE(picture->image().isNull(), !retain);
    QTRY_COMPARE(picture->status(), qamui::Image::Status::Error);
    QVERIFY(picture->image().isNull());
    QCOMPARE(picture->progress(), 0.0);
    QCOMPARE(server.requests, 2);

    // Superseding a download drops it: the later source wins.
    picture->setSource(QUrl(server.url("/also-missing.png")));
    picture->setSource(QUrl(server.url("/picture.png")));   // cached: Ready at once
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
    QTest::qWait(50);   // the aborted reply must not change anything
    QCOMPARE(picture->status(), qamui::Image::Status::Ready);
}

QTEST_MAIN(TestImage)
#include "test_image.moc"
