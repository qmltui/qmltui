#include "AudioEngine.h"

#include "Playlist.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#if CYBER_AUDIO_HAVE_MULTIMEDIA
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QMediaMetaData>
#endif

using namespace cyber;

namespace {

constexpr int kTickMs = 33;
constexpr float kBandAttack = 0.6f;
constexpr float kBandRelease = 0.2f;
constexpr float kPeakGravity = 0.02f;
constexpr float kVuAttack = 0.65f;
constexpr float kVuRelease = 0.12f;
constexpr float kScopeFade = 0.8f;
constexpr float kScopeBlend = 0.6f;          // weight of the newest trigger-aligned scope window
constexpr float kIdleThreshold = 0.005f;
constexpr int kMaxStaleTicks = 6;            // ~200 ms without PCM before levels decay
constexpr size_t kMaxPendingSeconds = 1;     // drop older PCM beyond this backlog
// Scope low-pass presets, highest first; 0 means off.
constexpr double kLowPassPresets[] = { 0.0, 8000.0, 4000.0, 2000.0, 1000.0, 600.0, 300.0, 150.0 };
constexpr int kLowPassPresetCount = int(sizeof(kLowPassPresets) / sizeof(kLowPassPresets[0]));

template <size_t N>
QVariantList toVariantList(const std::array<float, N>& values)
{
    QVariantList list;
    list.reserve(int(N));
    for (float v : values)
        list.append(double(v));
    return list;
}

template <size_t N>
QList<double> toDoubleList(const std::array<float, N>& values)
{
    QList<double> list;
    list.reserve(int(N));
    for (float v : values)
        list.append(double(v));
    return list;
}

} // namespace

AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    m_timer.setInterval(kTickMs);
    connect(&m_timer, &QTimer::timeout, this, &AudioEngine::tick);
    resetLevels();
    setStatus(QStringLiteral("NO SOURCE │ pass audio files or a folder on the command line, or press M for the microphone"));
}

AudioEngine::~AudioEngine()
{
    stopSources();
}

QString AudioEngine::sourceName() const
{
    switch (m_source) {
    case Source::File: return QStringLiteral("FILE");
    case Source::Microphone: return QStringLiteral("MIC");
    case Source::Demo: return QStringLiteral("DEMO");
    case Source::Idle: break;
    }
    return QStringLiteral("IDLE");
}

// ---------------------------------------------------------------------------
// Tick: smoothing shared by every source
// ---------------------------------------------------------------------------

void AudioEngine::tick()
{
    std::array<float, kBandCount> targets{};
    float vuTargetLeft = 0.f;
    float vuTargetRight = 0.f;
    bool haveWaveform = false;

    if (m_source == Source::Idle) {
        m_beatPulse = 0.f;
    } else if (m_source == Source::Demo) {
        if (m_playing) {
            const DemoSynth::Frame frame = m_synth.step();
            targets = frame.bandTargets;
            m_beatPulse = frame.beatPulse;
            m_scopeLeft = frame.scopeLeft;
            m_scopeRight = frame.scopeRight;
            haveWaveform = true;
        } else {
            m_beatPulse = std::max(0.f, m_beatPulse - 0.12f);
        }
    } else {
        m_beatPulse = 0.f;
        if (m_playing) {
            // Feed one tick's worth of queued PCM to the analyzer.
            const size_t framesPerTick = size_t(m_analyzer.sampleRate()) * kTickMs / 1000;
            const size_t available = (m_pending.size() - m_pendingRead) / 2;
            const size_t take = std::min(available, framesPerTick);
            if (take > 0) {
                m_analyzer.pushInterleaved(m_pending.data() + m_pendingRead, int(take), 2);
                m_pendingRead += take * 2;
                if (m_pendingRead >= m_pending.size()) {
                    m_pending.clear();
                    m_pendingRead = 0;
                } else if (m_pendingRead > m_pending.size() / 2) {
                    m_pending.erase(m_pending.begin(), m_pending.begin() + qsizetype(m_pendingRead));
                    m_pendingRead = 0;
                }
            }

            if (m_analyzer.hasNewData()) {
                const Analysis analysis = m_analyzer.analyze();
                m_lastTargets = analysis.bands;
                // A full-scale sine has RMS 1/sqrt(2); scale so it reads 0 dB on the meter.
                m_lastVuLeft = std::min(1.f, analysis.rmsLeft * std::numbers::sqrt2_v<float>);
                m_lastVuRight = std::min(1.f, analysis.rmsRight * std::numbers::sqrt2_v<float>);
                // Windows are trigger-aligned, so blending consecutive ones averages
                // out sample noise without smearing the waveform shape.
                const float blend = m_staleTicks > kMaxStaleTicks ? 1.f : kScopeBlend;
                for (int i = 0; i < kScopePoints; ++i) {
                    m_scopeLeft[i] += (analysis.scopeLeft[i] - m_scopeLeft[i]) * blend;
                    m_scopeRight[i] += (analysis.scopeRight[i] - m_scopeRight[i]) * blend;
                }
                m_staleTicks = 0;
                haveWaveform = true;
            } else {
                ++m_staleTicks;
            }
            // Hold the last analysis briefly across gaps in delivery; decay after that.
            if (m_staleTicks <= kMaxStaleTicks) {
                targets = m_lastTargets;
                vuTargetLeft = m_lastVuLeft;
                vuTargetRight = m_lastVuRight;
                haveWaveform = haveWaveform || m_staleTicks > 0;
            }
        }
    }

    bool active = false;
    for (int i = 0; i < kBandCount; ++i) {
        const float target = targets[i];
        float& level = m_levels[i];
        const float rate = target > level ? kBandAttack : kBandRelease;
        level += (target - level) * rate;
        if (level < kIdleThreshold)
            level = 0.f;
        float& peak = m_peaks[i];
        peak = level >= peak ? level : std::max(level, peak - kPeakGravity);
        if (peak < kIdleThreshold)
            peak = 0.f;
        active = active || level > 0.f || peak > 0.f;
    }

    if (m_source == Source::Demo && m_playing) {
        vuTargetLeft = std::min(1.f, m_levels[1] * 0.55f + m_levels[8] * 0.35f + m_levels[16] * 0.10f);
        vuTargetRight = std::min(1.f, m_levels[2] * 0.50f + m_levels[10] * 0.35f + m_levels[18] * 0.15f);
    }
    auto smoothVu = [](float& value, float target) {
        value = target > value ? value * (1.f - kVuAttack) + target * kVuAttack
                               : value * (1.f - kVuRelease) + target * kVuRelease;
        if (value < kIdleThreshold)
            value = 0.f;
    };
    smoothVu(m_vuLeft, vuTargetLeft);
    smoothVu(m_vuRight, vuTargetRight);
    active = active || m_vuLeft > 0.f || m_vuRight > 0.f || m_beatPulse > 0.f;

    if (!haveWaveform) {
        bool scopeActive = false;
        for (int i = 0; i < kScopePoints; ++i) {
            m_scopeLeft[i] *= kScopeFade;
            m_scopeRight[i] *= kScopeFade;
            if (std::abs(m_scopeLeft[i]) < kIdleThreshold)
                m_scopeLeft[i] = 0.f;
            if (std::abs(m_scopeRight[i]) < kIdleThreshold)
                m_scopeRight[i] = 0.f;
            scopeActive = scopeActive || m_scopeLeft[i] != 0.f || m_scopeRight[i] != 0.f;
        }
        active = active || scopeActive;
    }

    m_bandLevels = toVariantList(m_levels);
    m_peakLevels = toVariantList(m_peaks);
    m_waveformLeft = toDoubleList(m_scopeLeft);
    m_waveformRight = toDoubleList(m_scopeRight);
    updateMonoAndCorrelation();
    emit levelsChanged();

    // Once paused output has settled, stop ticking so the frame stays quiet.
    if (!m_playing && !active)
        m_timer.stop();
}

void AudioEngine::resetLevels()
{
    m_levels.fill(0.f);
    m_peaks.fill(0.f);
    m_scopeLeft.fill(0.f);
    m_scopeRight.fill(0.f);
    m_vuLeft = m_vuRight = m_beatPulse = 0.f;
    m_bandLevels = toVariantList(m_levels);
    m_peakLevels = toVariantList(m_peaks);
    m_waveformLeft = toDoubleList(m_scopeLeft);
    m_waveformRight = toDoubleList(m_scopeRight);
    updateMonoAndCorrelation();
    emit levelsChanged();
}

void AudioEngine::updateMonoAndCorrelation()
{
    m_waveformMono.resize(kScopePoints);
    double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
    for (int i = 0; i < kScopePoints; ++i) {
        const double l = m_scopeLeft[i];
        const double r = m_scopeRight[i];
        m_waveformMono[i] = 0.5 * (l + r);
        sumLR += l * r;
        sumLL += l * l;
        sumRR += r * r;
    }
    const double denom = std::sqrt(sumLL * sumRR);
    const float target = denom > 1e-9 ? float(std::clamp(sumLR / denom, -1.0, 1.0)) : m_correlation;
    m_correlation += (target - m_correlation) * 0.25f;   // light smoothing for a readable number
}

// ---------------------------------------------------------------------------
// State helpers
// ---------------------------------------------------------------------------

void AudioEngine::setPlaying(bool playing)
{
    const bool changed = m_playing != playing;
    m_playing = playing;
    if (changed)
        emit playingChanged();
    if (playing) {
        // Produce data synchronously so the very first frame after a source change
        // already shows levels instead of waiting for the first timer tick.
        tick();
        m_timer.start();
    }
}

void AudioEngine::setSource(Source source)
{
    if (m_source == source)
        return;
    m_source = source;
    emit sourceChanged();
}

void AudioEngine::setStatus(const QString& status)
{
    if (m_statusText == status)
        return;
    m_statusText = status;
    emit statusChanged();
}

void AudioEngine::setError(const QString& error)
{
    if (m_errorText == error)
        return;
    m_errorText = error;
    emit statusChanged();
}

void AudioEngine::clearError()
{
    setError(QString());
}

void AudioEngine::setPosition(qint64 position)
{
    if (m_position == position)
        return;
    m_position = position;
    emit positionChanged();
}

void AudioEngine::setDuration(qint64 duration, bool seekable)
{
    if (m_duration == duration && m_seekable == seekable)
        return;
    m_duration = duration;
    m_seekable = seekable;
    emit durationChanged();
}

void AudioEngine::setVolume(qreal volume)
{
    const qreal clamped = std::clamp(volume, 0.0, 1.0);
    if (qFuzzyCompare(m_volume, clamped))
        return;
    m_volume = clamped;
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_audioOutput)
        m_audioOutput->setVolume(float(m_volume));
#endif
    emit volumeChanged();
}

void AudioEngine::adjustVolume(qreal delta)
{
    setVolume(m_volume + delta);
}

void AudioEngine::setScopeLowPassHz(qreal cutoffHz)
{
    const double before = m_analyzer.scopeLowPassHz();
    m_analyzer.setScopeLowPassHz(cutoffHz);
    if (m_analyzer.scopeLowPassHz() != before)
        emit scopeLowPassChanged();
}

void AudioEngine::stepScopeLowPass(int direction)
{
    // Find the current preset (nearest), then move one step. Index 0 is "off".
    const double current = m_analyzer.scopeLowPassHz();
    int index = 0;
    for (int i = 0; i < kLowPassPresetCount; ++i)
        if (std::abs(kLowPassPresets[i] - current) < std::abs(kLowPassPresets[index] - current))
            index = i;
    // direction -1 lowers the cutoff (higher preset index); +1 raises it toward off.
    index = std::clamp(index - direction, 0, kLowPassPresetCount - 1);
    setScopeLowPassHz(kLowPassPresets[index]);
}

QString AudioEngine::scopeLowPassLabel() const
{
    const double hz = m_analyzer.scopeLowPassHz();
    if (hz <= 0.0)
        return QStringLiteral("off");
    if (hz >= 1000.0)
        return QStringLiteral("%1 kHz").arg(hz / 1000.0, 0, 'g', 3);
    return QStringLiteral("%1 Hz").arg(int(std::lround(hz)));
}

QString AudioEngine::formatTime(qint64 ms)
{
    const qint64 totalSeconds = std::max<qint64>(0, ms) / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds / 60) % 60;
    const qint64 seconds = totalSeconds % 60;
    const QString mmss = QStringLiteral("%1:%2")
                             .arg(minutes, 2, 10, QLatin1Char('0'))
                             .arg(seconds, 2, 10, QLatin1Char('0'));
    return hours > 0 ? QStringLiteral("%1:%2").arg(hours).arg(mmss) : mmss;
}

// ---------------------------------------------------------------------------
// Transport
// ---------------------------------------------------------------------------

void AudioEngine::play()
{
    if (m_source == Source::Idle)
        return;
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_source == Source::File && m_player) {
        m_player->play();
        setPlaying(true);
        return;
    }
    if (m_source == Source::Microphone && m_micSource) {
        m_micSource->resume();
        setPlaying(true);
        return;
    }
#endif
    setPlaying(true);
}

void AudioEngine::pause()
{
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_source == Source::File && m_player)
        m_player->pause();
    if (m_source == Source::Microphone && m_micSource)
        m_micSource->suspend();
#endif
    setPlaying(false);
}

void AudioEngine::togglePlayback()
{
    if (m_playing)
        pause();
    else
        play();
}

void AudioEngine::seek(qint64 deltaMs)
{
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_source != Source::File || !m_player || !m_seekable)
        return;
    const qint64 target = std::clamp<qint64>(m_player->position() + deltaMs, 0, std::max<qint64>(0, m_duration));
    m_player->setPosition(target);
    setPosition(target);
#else
    Q_UNUSED(deltaMs);
#endif
}

void AudioEngine::next()
{
    if (m_playlist.isEmpty())
        return;
    if (m_source != Source::File) {
        useFile();
        return;
    }
    loadTrack((m_trackIndex + 1) % int(m_playlist.size()));
}

void AudioEngine::previous()
{
    if (m_playlist.isEmpty())
        return;
    if (m_source != Source::File) {
        useFile();
        return;
    }
    loadTrack((m_trackIndex - 1 + int(m_playlist.size())) % int(m_playlist.size()));
}

// ---------------------------------------------------------------------------
// Sources
// ---------------------------------------------------------------------------

void AudioEngine::useDemo()
{
    stopSources();
    m_synth.reset();
    m_analyzer.clear();
    setSource(Source::Demo);
    clearError();
    setStatus(QStringLiteral("DEMO │ SYNTHWAVE RUNNER │ 138 BPM"));
    setPlaying(true);
}

void AudioEngine::loadPlaylist(const QStringList& inputs)
{
    const PlaylistScan scan = collectPlaylist(inputs);
    if (!multimediaAvailable()) {
        setError(QStringLiteral("Qt Multimedia not available in this build; use --demo for the showcase"));
        return;
    }
    if (scan.entries.isEmpty()) {
        setError(scan.warnings.isEmpty() ? QStringLiteral("No playable files given")
                                         : scan.warnings.join(QStringLiteral(" · ")));
        return;
    }
    m_playlist = scan.entries;
    m_failedInARow = 0;
    if (!scan.warnings.isEmpty())
        setError(scan.warnings.join(QStringLiteral(" · ")));
    else
        clearError();
    useFile();
}

void AudioEngine::useFile()
{
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_playlist.isEmpty()) {
        setError(QStringLiteral("No playlist loaded"));
        return;
    }
    stopSources();
    ensurePlayer();
    m_analyzer.setSampleRate(48000);
    m_analyzer.clear();
    setSource(Source::File);
    loadTrack(m_trackIndex < 0 ? 0 : m_trackIndex);
#else
    setError(QStringLiteral("Qt Multimedia not available in this build"));
#endif
}

void AudioEngine::useMicrophone()
{
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (!m_devices) {
        m_devices = new QMediaDevices(this);
        connect(m_devices, &QMediaDevices::audioInputsChanged, this, &AudioEngine::refreshInputDevice);
    }
    refreshInputDevice();
    const QAudioDevice device = QMediaDevices::defaultAudioInput();
    if (device.isNull()) {
        setError(QStringLiteral("No microphone detected"));
        return;
    }

    stopSources();

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format))
        format = device.preferredFormat();
    m_micFormat = format;
    m_analyzer.setSampleRate(format.sampleRate());
    m_analyzer.clear();

    m_micSource = new QAudioSource(device, format, this);
    connect(m_micSource, &QAudioSource::stateChanged, this, [this](QtAudio::State state) {
        if (state == QtAudio::StoppedState && m_micSource && m_micSource->error() != QtAudio::NoError)
            setError(QStringLiteral("Microphone stopped (error %1)").arg(int(m_micSource->error())));
    });
    m_micDevice = m_micSource->start();
    if (!m_micDevice) {
        setError(QStringLiteral("Could not open %1").arg(device.description()));
        m_micSource->deleteLater();
        m_micSource = nullptr;
        return;
    }
    connect(m_micDevice, &QIODevice::readyRead, this, &AudioEngine::onMicReadyRead);

    setSource(Source::Microphone);
    clearError();
    setStatus(QStringLiteral("MIC │ %1 │ %2 Hz").arg(device.description()).arg(format.sampleRate()));
    setPlaying(true);
#else
    setError(QStringLiteral("Qt Multimedia not available in this build"));
#endif
}

void AudioEngine::stopSources()
{
    m_pending.clear();
    m_pendingRead = 0;
    m_staleTicks = kMaxStaleTicks + 1;
    m_lastTargets.fill(0.f);
    m_lastVuLeft = m_lastVuRight = 0.f;
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (m_player)
        m_player->stop();
    if (m_micSource) {
        m_micSource->stop();
        m_micSource->deleteLater();
        m_micSource = nullptr;
        m_micDevice = nullptr;
    }
#endif
    setPosition(0);
    setDuration(0, false);
}

void AudioEngine::loadTrack(int index)
{
#if CYBER_AUDIO_HAVE_MULTIMEDIA
    if (!m_player || m_playlist.isEmpty())
        return;
    m_trackIndex = std::clamp(index, 0, int(m_playlist.size()) - 1);
    const QUrl url = m_playlist.at(m_trackIndex);
    m_title = displayNameFor(url);
    m_artist.clear();
    m_buffersForTrack = 0;
    m_analyzer.clear();
    emit trackChanged();
    setStatus(QStringLiteral("FILE │ TRACK %1/%2").arg(m_trackIndex + 1).arg(m_playlist.size()));

    m_player->setSource(url);
    m_player->play();
    setPlaying(true);

    // QAudioBufferOutput is only served by the FFmpeg backend. Tell the user when
    // playback runs but no PCM ever arrives.
    QTimer::singleShot(1500, this, [this, index = m_trackIndex] {
        if (m_source == Source::File && m_trackIndex == index && m_playing
            && m_buffersForTrack == 0 && m_errorText.isEmpty())
            setError(QStringLiteral("No audio data received: audio device busy (another player running?) "
                                    "or Qt Multimedia FFmpeg backend missing"));
    });
#else
    Q_UNUSED(index);
#endif
}

void AudioEngine::enqueuePcm(const float* interleavedStereo, int frames)
{
    if (!interleavedStereo || frames <= 0)
        return;
    m_pending.insert(m_pending.end(), interleavedStereo, interleavedStereo + size_t(frames) * 2);
    // Keep the backlog bounded so the display never lags far behind the sound.
    const size_t maxPending = size_t(m_analyzer.sampleRate()) * kMaxPendingSeconds * 2;
    if (m_pending.size() - m_pendingRead > maxPending) {
        m_pending.erase(m_pending.begin(), m_pending.end() - qsizetype(maxPending));
        m_pendingRead = 0;
    }
}

// ---------------------------------------------------------------------------
// Qt Multimedia integration
// ---------------------------------------------------------------------------

#if CYBER_AUDIO_HAVE_MULTIMEDIA

void AudioEngine::ensurePlayer()
{
    if (m_player)
        return;
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_audioOutput->setVolume(float(m_volume));
    m_player->setAudioOutput(m_audioOutput);

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);
    m_bufferOutput = new QAudioBufferOutput(format, this);
    m_player->setAudioBufferOutput(m_bufferOutput);

    // Buffers are emitted from a decoder thread; this object lives on the main thread,
    // so the connection is queued and the (implicitly shared) buffer crosses safely.
    connect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived, this, &AudioEngine::ingestBuffer);
    connect(m_player, &QMediaPlayer::positionChanged, this, &AudioEngine::setPosition);
    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 d) { setDuration(d, m_player->isSeekable()); });
    connect(m_player, &QMediaPlayer::seekableChanged, this, [this](bool s) { setDuration(m_player->duration(), s); });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &AudioEngine::onMediaStatus);
    connect(m_player, &QMediaPlayer::errorOccurred, this, &AudioEngine::onPlayerError);
    connect(m_player, &QMediaPlayer::metaDataChanged, this, &AudioEngine::onMetaData);
}

void AudioEngine::ingestBuffer(const QAudioBuffer& buffer)
{
    if (!buffer.isValid())
        return;
    const QAudioFormat format = buffer.format();
    const int channels = format.channelCount();
    const int frames = int(buffer.frameCount());
    if (channels <= 0 || frames <= 0)
        return;
    if (m_source == Source::File)
        ++m_buffersForTrack;
    m_failedInARow = 0;
    m_analyzer.setSampleRate(format.sampleRate());

    // Normalise every format to interleaved stereo float.
    m_convert.resize(size_t(frames) * 2);
    if (format.sampleFormat() == QAudioFormat::Float) {
        const float* src = buffer.constData<float>();
        for (int f = 0; f < frames; ++f) {
            const float* frame = src + qsizetype(f) * channels;
            m_convert[size_t(f) * 2] = frame[0];
            m_convert[size_t(f) * 2 + 1] = channels > 1 ? frame[1] : frame[0];
        }
    } else {
        // Generic path for Int16 / Int32 / UInt8 buffers.
        const auto* bytes = buffer.constData<char>();
        const int bytesPerSample = format.bytesPerSample();
        for (int f = 0; f < frames; ++f) {
            const char* frame = bytes + qsizetype(f) * channels * bytesPerSample;
            const float left = format.normalizedSampleValue(frame);
            const float right = channels > 1 ? format.normalizedSampleValue(frame + bytesPerSample) : left;
            m_convert[size_t(f) * 2] = left;
            m_convert[size_t(f) * 2 + 1] = right;
        }
    }
    enqueuePcm(m_convert.data(), frames);
}

void AudioEngine::onMicReadyRead()
{
    if (!m_micDevice)
        return;
    const QByteArray bytes = m_micDevice->readAll();
    if (bytes.isEmpty())
        return;
    ingestBuffer(QAudioBuffer(bytes, m_micFormat));
}

void AudioEngine::onMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia && m_source == Source::File)
        next();
}

void AudioEngine::onPlayerError(QMediaPlayer::Error error, const QString& message)
{
    if (error == QMediaPlayer::NoError || m_source != Source::File)
        return;
    setError(QStringLiteral("ERROR │ %1 │ %2").arg(m_title, message.trimmed()));
    ++m_failedInARow;
    if (m_failedInARow >= int(m_playlist.size())) {
        pause();
        return;
    }
    QTimer::singleShot(1500, this, [this] {
        if (m_source == Source::File && !m_errorText.isEmpty())
            next();
    });
}

void AudioEngine::onMetaData()
{
    if (!m_player || m_trackIndex < 0)
        return;
    const QMediaMetaData meta = m_player->metaData();
    const QString title = meta.stringValue(QMediaMetaData::Title).trimmed();
    QString artist = meta.stringValue(QMediaMetaData::ContributingArtist).trimmed();
    if (artist.isEmpty())
        artist = meta.stringValue(QMediaMetaData::AlbumArtist).trimmed();
    if (artist.isEmpty())
        artist = meta.stringValue(QMediaMetaData::Author).trimmed();
    const QString newTitle = title.isEmpty() ? displayNameFor(m_playlist.at(m_trackIndex)) : title;
    if (newTitle == m_title && artist == m_artist)
        return;
    m_title = newTitle;
    m_artist = artist;
    emit trackChanged();
}

void AudioEngine::refreshInputDevice()
{
    const bool has = !QMediaDevices::defaultAudioInput().isNull();
    if (has == m_hasInputDevice)
        return;
    m_hasInputDevice = has;
    emit hasInputDeviceChanged();
}

#endif // CYBER_AUDIO_HAVE_MULTIMEDIA
