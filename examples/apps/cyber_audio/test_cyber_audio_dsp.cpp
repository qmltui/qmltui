#include "AudioEngine.h"
#include "DemoSynth.h"
#include "ScopeArt.h"
#include "WaveformTrace.h"
#include "Playlist.h"
#include "SpectrumAnalyzer.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <Qtui/Core/items/Canvas.h>
#include <Qtui/Core/primitives/TextGridBuffer.h>

#include <cmath>
#include <numbers>

using namespace cyber;

namespace {

// Interleaved stereo sine, one frequency per channel.
std::vector<float> stereoSine(int frames, int rate, double hzLeft, double ampLeft,
                              double hzRight, double ampRight)
{
    std::vector<float> data(size_t(frames) * 2);
    for (int n = 0; n < frames; ++n) {
        const double t = double(n) / rate;
        data[size_t(n) * 2] = float(ampLeft * std::sin(2.0 * std::numbers::pi * hzLeft * t));
        data[size_t(n) * 2 + 1] = float(ampRight * std::sin(2.0 * std::numbers::pi * hzRight * t));
    }
    return data;
}

int argMax(const std::array<float, kBandCount>& bands)
{
    int best = 0;
    for (int b = 1; b < kBandCount; ++b)
        if (bands[b] > bands[best])
            best = b;
    return best;
}

bool touch(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write("x") == 1;
}

} // namespace

class TestCyberAudioDsp : public QObject
{
    Q_OBJECT

private slots:
    void bandEdgesAreLogSpaced();
    void labelsFitUnderNarrowBars();
    void fftResolvesBinAlignedCosine();
    void sineIsDetectedInItsBand();
    void rmsFollowsAmplitude();
    void monoIsDuplicatedToBothChannels();
    void scopeWindowIsTriggerAligned();
    void scopeIsLowPassFiltered();
    void silenceIsFloor();
    void demoSynthIsDeterministic();
    void playlistScanFiltersAndSorts();
    void playlistReportsMissingInputs();
    void engineRunsDemoAndSettlesWhenPaused();
    void engineRejectsUnavailableSources();
    void engineFormatsTime();
    void engineStepsScopeLowPass();
    void correlationTracksStereoWidth();
    void shapesDrawIntoBrailleCanvas();
};

void TestCyberAudioDsp::bandEdgesAreLogSpaced()
{
    QCOMPARE(SpectrumAnalyzer::bandLowerEdgeHz(0), kBandLowHz);
    QVERIFY(std::abs(SpectrumAnalyzer::bandLowerEdgeHz(kBandCount) - kBandHighHz) < 1e-6);

    const double ratio = SpectrumAnalyzer::bandLowerEdgeHz(1) / SpectrumAnalyzer::bandLowerEdgeHz(0);
    for (int b = 1; b < kBandCount; ++b) {
        const double r = SpectrumAnalyzer::bandLowerEdgeHz(b + 1) / SpectrumAnalyzer::bandLowerEdgeHz(b);
        QVERIFY2(std::abs(r - ratio) < 1e-9, qPrintable(QString::number(r)));
    }

    const int band1k = SpectrumAnalyzer::bandIndexForFrequency(1000.0);
    QVERIFY(band1k >= 10 && band1k <= 12);
    QCOMPARE(SpectrumAnalyzer::bandIndexForFrequency(20.0), -1);
    QCOMPARE(SpectrumAnalyzer::bandIndexForFrequency(40.0), 0);
    QCOMPARE(SpectrumAnalyzer::bandIndexForFrequency(17999.0), kBandCount - 1);
    QCOMPARE(SpectrumAnalyzer::bandIndexForFrequency(18000.0), -1);

    for (int b = 0; b < kBandCount; ++b)
        QCOMPARE(SpectrumAnalyzer::bandIndexForFrequency(SpectrumAnalyzer::bandCenterHz(b)), b);
}

void TestCyberAudioDsp::labelsFitUnderNarrowBars()
{
    const QStringList labels = SpectrumAnalyzer::bandLabels();
    QCOMPARE(labels.size(), kBandCount);
    for (const QString& label : labels) {
        QVERIFY2(label.size() >= 1 && label.size() <= 3, qPrintable(label));
    }
    QCOMPARE(labels.first(), QStringLiteral("45"));
    QVERIFY(labels.last().endsWith(QLatin1Char('k')));
    QCOMPARE(SpectrumAnalyzer::formatFrequencyLabel(1250.0), QStringLiteral("1k3"));
    QCOMPARE(SpectrumAnalyzer::formatFrequencyLabel(2000.0), QStringLiteral("2k"));
    QCOMPARE(SpectrumAnalyzer::formatFrequencyLabel(12408.0), QStringLiteral("12k"));
}

void TestCyberAudioDsp::fftResolvesBinAlignedCosine()
{
    const int n = 64;
    const int bin = 5;
    std::vector<std::complex<float>> data(n);
    for (int i = 0; i < n; ++i)
        data[i] = { float(std::cos(2.0 * std::numbers::pi * bin * i / n)), 0.f };
    SpectrumAnalyzer::fft(data);

    for (int k = 0; k <= n / 2; ++k) {
        const float mag = std::abs(data[k]);
        if (k == bin)
            QVERIFY2(std::abs(mag - n / 2.f) < 1e-3f, qPrintable(QString::number(mag)));
        else
            QVERIFY2(mag < 1e-3f, qPrintable(QString("bin %1 = %2").arg(k).arg(mag)));
    }
}

void TestCyberAudioDsp::sineIsDetectedInItsBand()
{
    SpectrumAnalyzer analyzer(48000);
    const auto pcm = stereoSine(4096, 48000, 1000.0, 0.5, 1000.0, 0.5);
    analyzer.pushInterleaved(pcm.data(), 4096, 2);
    QVERIFY(analyzer.hasNewData());

    const Analysis a = analyzer.analyze();
    QVERIFY(!analyzer.hasNewData());

    const int expected = SpectrumAnalyzer::bandIndexForFrequency(1000.0);
    QCOMPARE(argMax(a.bands), expected);
    QVERIFY(a.bands[expected] > 0.8f);            // -6 dBFS sine sits high in the 0..1 range
    QVERIFY(a.bands[expected - 1] < a.bands[expected]);
    QVERIFY(a.bands[expected + 1] < a.bands[expected]);
    for (int b = 0; b < kBandCount; ++b) {
        if (std::abs(b - expected) >= 3)
            QVERIFY2(a.bands[b] < 0.35f, qPrintable(QString("band %1 = %2").arg(b).arg(a.bands[b])));
    }
    QCOMPARE(a.scopeLeft.size(), size_t(kScopePoints));
}

void TestCyberAudioDsp::rmsFollowsAmplitude()
{
    SpectrumAnalyzer analyzer(48000);
    const auto pcm = stereoSine(48000, 48000, 1000.0, 0.5, 440.0, 0.25);
    analyzer.pushInterleaved(pcm.data(), 48000, 2);
    const Analysis a = analyzer.analyze();

    QVERIFY2(std::abs(a.rmsLeft - 0.5f / std::numbers::sqrt2_v<float>) < 0.01f, qPrintable(QString::number(a.rmsLeft)));
    QVERIFY2(std::abs(a.rmsRight - 0.25f / std::numbers::sqrt2_v<float>) < 0.01f, qPrintable(QString::number(a.rmsRight)));

    // RMS accumulators reset after each analysis.
    const Analysis b = analyzer.analyze();
    QCOMPARE(b.rmsLeft, 0.f);
    QCOMPARE(b.rmsRight, 0.f);
}

void TestCyberAudioDsp::monoIsDuplicatedToBothChannels()
{
    SpectrumAnalyzer analyzer(44100);
    std::vector<float> mono(4096);
    for (int n = 0; n < 4096; ++n)
        mono[n] = float(0.3 * std::sin(2.0 * std::numbers::pi * 200.0 * n / 44100.0));
    analyzer.pushInterleaved(mono.data(), 4096, 1);
    const Analysis a = analyzer.analyze();
    QCOMPARE(a.rmsLeft, a.rmsRight);
    QCOMPARE(a.scopeLeft, a.scopeRight);
    QCOMPARE(argMax(a.bands), SpectrumAnalyzer::bandIndexForFrequency(200.0));
}

void TestCyberAudioDsp::scopeWindowIsTriggerAligned()
{
    SpectrumAnalyzer analyzer(48000);
    // Feed the same 100 Hz tone in three chunks of unrelated length: without a trigger
    // the window phase would differ every time.
    const auto pcm = stereoSine(48000, 48000, 100.0, 0.8, 100.0, 0.8);
    std::array<float, kScopePoints> previous{};
    const int chunks[] = { 4096, 1337, 2911 };
    int offset = 0;
    for (int c = 0; c < 3; ++c) {
        analyzer.pushInterleaved(pcm.data() + size_t(offset) * 2, chunks[c], 2);
        offset += chunks[c];
        const Analysis a = analyzer.analyze();
        QVERIFY(a.scopeTriggered);
        // The window starts on a rising zero crossing...
        QVERIFY2(std::abs(a.scopeLeft[0]) < 0.02f, qPrintable(QString::number(a.scopeLeft[0])));
        QVERIFY(a.scopeLeft[1] > a.scopeLeft[0]);
        // ...and therefore matches the previous window point for point. A threshold
        // trigger may land one frame either side of an exact zero sample; at 100 Hz and
        // amplitude 0.8 that shifts a point by at most 0.8 * 2pi * 100 / 48000 ~ 0.011
        // per frame, so allow a one-frame slip.
        if (c > 0) {
            for (int i = 0; i < kScopePoints; ++i)
                QVERIFY2(std::abs(a.scopeLeft[i] - previous[i]) < 0.02f,
                         qPrintable(QString("i=%1 %2 vs %3").arg(i).arg(a.scopeLeft[i]).arg(previous[i])));
        }
        previous = a.scopeLeft;
    }

    // Silence has no crossing: the window is simply the newest frames.
    SpectrumAnalyzer quiet(48000);
    std::vector<float> zeros(4096 * 2, 0.f);
    quiet.pushInterleaved(zeros.data(), 4096, 2);
    QVERIFY(!quiet.analyze().scopeTriggered);
}

void TestCyberAudioDsp::scopeIsLowPassFiltered()
{
    // 100 Hz at 0.5 plus 5 kHz "noise" at 0.4 on both channels: with a 300 Hz cutoff
    // the scope should show essentially the 100 Hz sine alone, while the FFT still sees
    // both tones. With the filter off the raw mix comes through.
    SpectrumAnalyzer analyzer(48000);
    QCOMPARE(analyzer.scopeLowPassHz(), 0.0);
    std::vector<float> pcm(48000 * 2);
    for (int n = 0; n < 48000; ++n) {
        const double t = double(n) / 48000.0;
        const float v = float(0.5 * std::sin(2.0 * std::numbers::pi * 100.0 * t)
                              + 0.4 * std::sin(2.0 * std::numbers::pi * 5000.0 * t));
        pcm[size_t(n) * 2] = v;
        pcm[size_t(n) * 2 + 1] = v;
    }
    analyzer.pushInterleaved(pcm.data(), 48000, 2);
    const Analysis raw = analyzer.analyze();
    float rawStep = 0.f;
    for (int i = 1; i < kScopePoints; ++i)
        rawStep = std::max(rawStep, std::abs(raw.scopeLeft[i] - raw.scopeLeft[i - 1]));
    QVERIFY2(rawStep > 0.3f, qPrintable(QString::number(rawStep)));   // 5 kHz is visible unfiltered

    analyzer.setScopeLowPassHz(300.0);
    QCOMPARE(analyzer.scopeLowPassHz(), 300.0);
    analyzer.pushInterleaved(pcm.data(), 48000, 2);
    const Analysis a = analyzer.analyze();

    // Both tones are in the spectrum.
    QVERIFY(a.bands[SpectrumAnalyzer::bandIndexForFrequency(100.0)] > 0.7f);
    QVERIFY(a.bands[SpectrumAnalyzer::bandIndexForFrequency(5000.0)] > 0.7f);

    // The scope trace is smooth: consecutive points (4 frames apart) never jump by
    // more than the 100 Hz slope allows, which the unfiltered 5 kHz component would
    // violate by an order of magnitude.
    float maxStep = 0.f;
    float peak = 0.f;
    for (int i = 1; i < kScopePoints; ++i) {
        maxStep = std::max(maxStep, std::abs(a.scopeLeft[i] - a.scopeLeft[i - 1]));
        peak = std::max(peak, std::abs(a.scopeLeft[i]));
    }
    QVERIFY2(maxStep < 0.06f, qPrintable(QString::number(maxStep)));   // 100 Hz: <= 0.5*2pi*100*4/48000 = 0.026
    QVERIFY2(peak > 0.4f && peak < 0.55f, qPrintable(QString::number(peak)));   // 100 Hz passes ~unchanged
    QVERIFY(a.scopeTriggered);

    // 0 switches the filter off again; out-of-range requests are clamped.
    analyzer.setScopeLowPassHz(0.0);
    QCOMPARE(analyzer.scopeLowPassHz(), 0.0);
    analyzer.setScopeLowPassHz(5.0);
    QCOMPARE(analyzer.scopeLowPassHz(), 20.0);
}

void TestCyberAudioDsp::silenceIsFloor()
{
    SpectrumAnalyzer analyzer;
    std::vector<float> zeros(4096 * 2, 0.f);
    analyzer.pushInterleaved(zeros.data(), 4096, 2);
    const Analysis a = analyzer.analyze();
    for (float level : a.bands)
        QCOMPARE(level, 0.f);
    QCOMPARE(a.rmsLeft, 0.f);
    QCOMPARE(a.rmsRight, 0.f);
}

void TestCyberAudioDsp::demoSynthIsDeterministic()
{
    DemoSynth a, b;
    DemoSynth::Frame first = a.step();
    b.step();

    // The first tick lands on a kick: bass bands are hot.
    QVERIFY(first.beatPulse > 0.99f);
    QVERIFY(first.bandTargets[0] > 0.5f);

    for (int i = 0; i < 100; ++i) {
        const DemoSynth::Frame fa = a.step();
        const DemoSynth::Frame fb = b.step();
        QCOMPARE(fa.bandTargets, fb.bandTargets);
        QCOMPARE(fa.scopeLeft, fb.scopeLeft);
        QCOMPARE(fa.beatPulse, fb.beatPulse);
        for (float t : fa.bandTargets)
            QVERIFY(t >= 0.03f && t <= 1.f);
        for (int s = 0; s < kScopePoints; ++s)
            QVERIFY(fa.scopeLeft[s] >= -1.f && fa.scopeLeft[s] <= 1.f);
    }

    a.reset();
    QCOMPARE(a.step().bandTargets, first.bandTargets);
}

void TestCyberAudioDsp::playlistScanFiltersAndSorts()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("sub"));
    QVERIFY(touch(dir.filePath("b.mp3")));
    QVERIFY(touch(dir.filePath("a.FLAC")));
    QVERIFY(touch(dir.filePath("notes.txt")));
    QVERIFY(touch(dir.filePath("sub/c.ogg")));
    QVERIFY(touch(dir.filePath("weird.xyz")));

    const PlaylistScan scan = collectPlaylist({ dir.path(), dir.filePath("weird.xyz") });
    QVERIFY2(scan.warnings.isEmpty(), qPrintable(scan.warnings.join("; ")));
    QCOMPARE(scan.entries.size(), 4);
    QCOMPARE(displayNameFor(scan.entries[0]), QStringLiteral("a"));
    QCOMPARE(displayNameFor(scan.entries[1]), QStringLiteral("b"));
    QCOMPARE(displayNameFor(scan.entries[2]), QStringLiteral("c"));
    QCOMPARE(displayNameFor(scan.entries[3]), QStringLiteral("weird"));
    for (const QUrl& url : scan.entries)
        QVERIFY(url.isLocalFile());
}

void TestCyberAudioDsp::playlistReportsMissingInputs()
{
    const PlaylistScan scan = collectPlaylist({ QStringLiteral("/definitely/not/here.mp3"),
                                                QStringLiteral("https://example.com/stream.mp3") });
    QCOMPARE(scan.entries.size(), 1);
    QVERIFY(!scan.entries.first().isLocalFile());
    QCOMPARE(displayNameFor(scan.entries.first()), QStringLiteral("stream.mp3"));
    QCOMPARE(scan.warnings.size(), 1);
    QVERIFY(scan.warnings.first().contains(QStringLiteral("Not found")));

    QTemporaryDir empty;
    const PlaylistScan none = collectPlaylist({ empty.path() });
    QVERIFY(none.entries.isEmpty());
    QCOMPARE(none.warnings.size(), 1);
}

void TestCyberAudioDsp::engineRunsDemoAndSettlesWhenPaused()
{
    AudioEngine engine;
    // Idle until a source is chosen: no ticks, nothing to show.
    QCOMPARE(engine.source(), AudioEngine::Source::Idle);
    QCOMPARE(engine.sourceName(), QStringLiteral("IDLE"));
    QVERIFY(!engine.playing());
    QVERIFY(engine.statusText().contains(QStringLiteral("press M")));
    {
        QSignalSpy idle(&engine, &AudioEngine::levelsChanged);
        engine.play();                                   // nothing to play
        QTest::qWait(200);
        QCOMPARE(idle.count(), 0);
        QVERIFY(!engine.playing());
    }

    engine.useDemo();
    QCOMPARE(engine.source(), AudioEngine::Source::Demo);
    QCOMPARE(engine.sourceName(), QStringLiteral("DEMO"));
    QVERIFY(engine.playing());
    QCOMPARE(engine.bandCount(), kBandCount);
    QCOMPARE(engine.bandLabels().size(), kBandCount);
    QCOMPARE(engine.bandLevels().size(), kBandCount);
    QCOMPARE(engine.waveformLeft().size(), kScopePoints);

    QSignalSpy levels(&engine, &AudioEngine::levelsChanged);
    QTRY_VERIFY_WITH_TIMEOUT(levels.count() >= 3, 1000);

    // The demo has energy right away (synchronous first tick on play).
    bool anyLevel = false;
    for (const QVariant& v : engine.bandLevels())
        anyLevel = anyLevel || v.toDouble() > 0.0;
    QVERIFY(anyLevel);

    engine.pause();
    QVERIFY(!engine.playing());
    // Levels decay (peak caps fall 0.02 per 33 ms tick, so up to ~1.7 s from a full
    // bar), then the tick timer stops: no more emissions after settling.
    QTest::qWait(2500);
    const int settled = levels.count();
    QTest::qWait(300);
    QCOMPARE(levels.count(), settled);
    for (const QVariant& v : engine.bandLevels())
        QCOMPARE(v.toDouble(), 0.0);
    QCOMPARE(engine.vuLeft(), 0.0);

    engine.play();
    QVERIFY(engine.playing());
    QTRY_VERIFY_WITH_TIMEOUT(levels.count() > settled + 2, 1000);
}

void TestCyberAudioDsp::engineRejectsUnavailableSources()
{
    AudioEngine engine;
    engine.loadPlaylist({});
    QCOMPARE(engine.source(), AudioEngine::Source::Idle);
    QVERIFY(!engine.errorText().isEmpty());
    QCOMPARE(engine.trackCount(), 0);

    engine.useDemo();
    QVERIFY(engine.errorText().isEmpty());

    // Transport controls are inert without a playlist.
    engine.next();
    engine.previous();
    engine.seek(5000);
    QCOMPARE(engine.source(), AudioEngine::Source::Demo);
    QCOMPARE(engine.position(), 0);

    engine.setVolume(1.7);
    QCOMPARE(engine.volume(), 1.0);
    engine.adjustVolume(-0.25);
    QCOMPARE(engine.volume(), 0.75);

    if (!AudioEngine::multimediaAvailable()) {
        engine.useMicrophone();
        QCOMPARE(engine.source(), AudioEngine::Source::Demo);
        QVERIFY(engine.errorText().contains(QStringLiteral("not available")));
    }
}

void TestCyberAudioDsp::engineStepsScopeLowPass()
{
    AudioEngine engine;
    QCOMPARE(engine.scopeLowPassHz(), 0.0);
    QCOMPARE(engine.scopeLowPassLabel(), QStringLiteral("off"));
    engine.stepScopeLowPass(1);                       // already off: stays off
    QCOMPARE(engine.scopeLowPassLabel(), QStringLiteral("off"));
    engine.stepScopeLowPass(-1);
    QCOMPARE(engine.scopeLowPassHz(), 8000.0);
    QCOMPARE(engine.scopeLowPassLabel(), QStringLiteral("8 kHz"));
    for (int i = 0; i < 5; ++i)
        engine.stepScopeLowPass(-1);
    QCOMPARE(engine.scopeLowPassHz(), 300.0);
    QCOMPARE(engine.scopeLowPassLabel(), QStringLiteral("300 Hz"));
    engine.stepScopeLowPass(-1);
    engine.stepScopeLowPass(-1);                      // clamps at the lowest preset
    QCOMPARE(engine.scopeLowPassHz(), 150.0);
    engine.setScopeLowPassHz(0.0);
    QCOMPARE(engine.scopeLowPassLabel(), QStringLiteral("off"));
}

void TestCyberAudioDsp::engineFormatsTime()
{
    QCOMPARE(AudioEngine::formatTime(0), QStringLiteral("00:00"));
    QCOMPARE(AudioEngine::formatTime(6000), QStringLiteral("00:06"));
    QCOMPARE(AudioEngine::formatTime(61999), QStringLiteral("01:01"));
    QCOMPARE(AudioEngine::formatTime(3723000), QStringLiteral("1:02:03"));
    QCOMPARE(AudioEngine::formatTime(-5), QStringLiteral("00:00"));
}

void TestCyberAudioDsp::correlationTracksStereoWidth()
{
    SpectrumAnalyzer analyzer(48000);
    const auto mono = stereoSine(4096, 48000, 440.0, 0.5, 440.0, 0.5);
    analyzer.pushInterleaved(mono.data(), 4096, 2);
    QVERIFY(analyzer.analyze().correlation > 0.99f);

    const auto wide = stereoSine(4096, 48000, 440.0, 0.5, 1000.0, 0.5);
    analyzer.pushInterleaved(wide.data(), 4096, 2);
    QVERIFY(std::abs(analyzer.analyze().correlation) < 0.2f);

    // Inverted channels correlate negatively.
    std::vector<float> inverted = mono;
    for (size_t i = 1; i < inverted.size(); i += 2)
        inverted[i] = -inverted[i];
    analyzer.pushInterleaved(inverted.data(), 4096, 2);
    QVERIFY(analyzer.analyze().correlation < -0.99f);
}

namespace {

// Paints the canvas and returns, per column, whether any Braille dot was drawn.
std::vector<bool> paintedColumns(qtui::Canvas& canvas, qtui::TextGridBuffer& buf)
{
    buf.resize(canvas.width(), canvas.height());
    const qtui::ClipRect clip{ 0, 0, canvas.width(), canvas.height() };
    canvas.paint(buf, 0, 0, clip);
    std::vector<bool> columns(size_t(canvas.width()), false);
    for (int y = 0; y < canvas.height(); ++y)
        for (int x = 0; x < canvas.width(); ++x)
            if (buf.cellAt(x, y).character().toByteArray() != QByteArray(" ")
                && !buf.cellAt(x, y).character().toByteArray().isEmpty())
                columns[size_t(x)] = true;
    return columns;
}

} // namespace

void TestCyberAudioDsp::shapesDrawIntoBrailleCanvas()
{
    qtui::Canvas canvas;
    canvas.setRenderMode(qtui::Canvas::RenderMode::Braille);
    canvas.setWidth(40);
    canvas.setHeight(10);
    canvas.setColor("white");

    QList<double> sine(kScopePoints);
    for (int i = 0; i < kScopePoints; ++i)
        sine[i] = 0.8 * std::sin(2.0 * std::numbers::pi * 3.0 * i / kScopePoints);

    // WaveformTrace spans the full width.
    auto* trace = new WaveformTrace(&canvas);
    trace->setParentItem(&canvas);
    trace->setSamples(sine);
    qtui::TextGridBuffer buf;
    auto cols = paintedColumns(canvas, buf);
    QVERIFY(cols.front());
    QVERIFY(cols.back());
    trace->setVisible(false);

    // A silent trace draws nothing.
    auto* silentTrace = new WaveformTrace(&canvas);
    silentTrace->setParentItem(&canvas);
    silentTrace->setSamples(QList<double>(kScopePoints, 0.0));
    cols = paintedColumns(canvas, buf);
    QVERIFY(std::none_of(cols.begin(), cols.end(), [](bool b) { return b; }));
    silentTrace->setVisible(false);

    // Ridge plot: history accumulates up to depth and paints within the inset.
    auto* ridge = new RidgePlot(&canvas);
    ridge->setParentItem(&canvas);
    ridge->setDepth(6);
    for (int k = 0; k < 10; ++k)
        ridge->setSamples(sine);
    QCOMPARE(ridge->samples().size(), kScopePoints);
    cols = paintedColumns(canvas, buf);
    QVERIFY(!cols.front());                      // 10 % inset on both sides
    QVERIFY(!cols.back());
    QVERIFY(cols[size_t(canvas.width() / 2)]);
    ridge->setVisible(false);

    // Persistence: after depth+2 pushes only depth windows are kept, and it draws.
    auto* glow = new PersistenceTrace(&canvas);
    glow->setParentItem(&canvas);
    glow->setDepth(4);
    for (int k = 0; k < 6; ++k)
        glow->setSamples(sine);
    cols = paintedColumns(canvas, buf);
    QVERIFY(cols.front() && cols.back());
}

QTEST_GUILESS_MAIN(TestCyberAudioDsp)
#include "test_cyber_audio_dsp.moc"
