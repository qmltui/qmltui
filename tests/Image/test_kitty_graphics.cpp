// Kitty graphics output from the Image item, verified through the memory
// backend: transmission and placement precede the placeholder cells, the
// cells carry the image id in their foreground, a source change frees the old
// upload, an unchanged frame re-sends nothing, and Auto falls back to
// half-block on a terminal without the protocol.
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QImage>
#include <QRegularExpression>

#include <TestRunner/MemoryTerminalBackend.h>
#include "Terminal.h"
#include "TerminalEngine.h"
#include "Image.h"
#include "TextGridBuffer.h"

using namespace qtui;

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
import Qtui
import Qtui.Image
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
        const auto transmit = out.find("\033_Gq=2,a=t,f=100,t=d,i=");
        const auto place = out.find("\033_Ga=p,U=1,q=2,i=");
        QVERIFY(transmit != std::string::npos);
        QVERIFY(place != std::string::npos);
        QVERIFY(transmit < place);   // upload before placement

        // The placement id matches the id carried by the placeholder cells.
        const QRegularExpression re("\\033_Ga=p,U=1,q=2,i=(\\d+),c=6,r=3");
        const auto m = re.match(QString::fromLatin1(out.data(), int(out.size())));
        QVERIFY2(m.hasMatch(), "placement not found with the expected cols/rows");
        const uint32_t id = m.captured(1).toUInt();
        uint32_t fg = 0;
        QCOMPARE(placeholderCellCount(engine, &fg), 18);   // 6x3 cells
        QCOMPARE(fg, 0xFF000000u | (id & 0xFFFFFFu));
        QVERIFY(place < out.find(std::string(kPlaceholderUtf8, 4)));  // cells after controls
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

    void sourceChangeFreesOldUpload() {
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
        QVERIFY(out.find("\033_Ga=d,d=I,q=2,i=" + std::to_string(firstId)) != std::string::npos);
        QVERIFY(out.find("\033_Gq=2,a=t") != std::string::npos);  // re-uploaded
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
