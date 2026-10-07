#pragma once

#include "DemoSynth.h"
#include "SpectrumAnalyzer.h"

#include <QObject>
#include <QQmlEngine>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>

#include <array>
#include <vector>

#if CYBER_AUDIO_HAVE_MULTIMEDIA
#include <QAudioFormat>
#include <QMediaPlayer>
class QAudioBuffer;
class QAudioBufferOutput;
class QAudioOutput;
class QAudioSource;
class QIODevice;
class QMediaDevices;
#endif

// Feeds the visualizer from one of three sources (procedural demo, media files,
// microphone) and owns all smoothing so QML only renders. Every level property
// notifies through a single levelsChanged() per 33 ms tick.
class AudioEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    // Idle: nothing selected yet (start-up without media). Demo is the procedural
    // synth used by the test scenarios and the --demo flag; it has no UI of its own.
    enum class Source { Idle, Demo, File, Microphone };
    Q_ENUM(Source)

    Q_PROPERTY(Source source READ source NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool multimediaAvailable READ multimediaAvailable CONSTANT)
    Q_PROPERTY(bool hasInputDevice READ hasInputDevice NOTIFY hasInputDeviceChanged)
    Q_PROPERTY(int bandCount READ bandCount CONSTANT)
    Q_PROPERTY(QStringList bandLabels READ bandLabels CONSTANT)

    Q_PROPERTY(QVariantList bandLevels READ bandLevels NOTIFY levelsChanged)
    Q_PROPERTY(QVariantList peakLevels READ peakLevels NOTIFY levelsChanged)
    Q_PROPERTY(qreal vuLeft READ vuLeft NOTIFY levelsChanged)
    Q_PROPERTY(qreal vuRight READ vuRight NOTIFY levelsChanged)
    Q_PROPERTY(qreal beatPulse READ beatPulse NOTIFY levelsChanged)
    // Plain double lists: QML hands them to WaveformTrace without per-element boxing.
    Q_PROPERTY(QList<double> waveformLeft READ waveformLeft NOTIFY levelsChanged)
    Q_PROPERTY(QList<double> waveformRight READ waveformRight NOTIFY levelsChanged)
    Q_PROPERTY(QList<double> waveformMono READ waveformMono NOTIFY levelsChanged)
    // Normalized L/R correlation of the scope window: +1 mono, 0 unrelated, -1 out of phase.
    Q_PROPERTY(qreal correlation READ correlation NOTIFY levelsChanged)
    // Scope display low-pass cutoff in Hz; 0 = off. Stepped through presets from QML.
    Q_PROPERTY(qreal scopeLowPassHz READ scopeLowPassHz WRITE setScopeLowPassHz NOTIFY scopeLowPassChanged)
    Q_PROPERTY(QString scopeLowPassLabel READ scopeLowPassLabel NOTIFY scopeLowPassChanged)

    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY durationChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(int trackIndex READ trackIndex NOTIFY trackChanged)
    Q_PROPERTY(int trackCount READ trackCount NOTIFY trackChanged)
    Q_PROPERTY(QString title READ title NOTIFY trackChanged)
    Q_PROPERTY(QString artist READ artist NOTIFY trackChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY statusChanged)

    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    Source source() const { return m_source; }
    QString sourceName() const;
    bool playing() const { return m_playing; }
    static bool multimediaAvailable() { return CYBER_AUDIO_HAVE_MULTIMEDIA != 0; }
    bool hasInputDevice() const { return m_hasInputDevice; }
    static int bandCount() { return cyber::kBandCount; }
    static QStringList bandLabels() { return cyber::SpectrumAnalyzer::bandLabels(); }

    QVariantList bandLevels() const { return m_bandLevels; }
    QVariantList peakLevels() const { return m_peakLevels; }
    qreal vuLeft() const { return m_vuLeft; }
    qreal vuRight() const { return m_vuRight; }
    QList<double> waveformMono() const { return m_waveformMono; }
    qreal correlation() const { return m_correlation; }
    qreal beatPulse() const { return m_beatPulse; }
    QList<double> waveformLeft() const { return m_waveformLeft; }
    QList<double> waveformRight() const { return m_waveformRight; }
    qreal scopeLowPassHz() const { return m_analyzer.scopeLowPassHz(); }
    void setScopeLowPassHz(qreal cutoffHz);
    QString scopeLowPassLabel() const;

    qint64 position() const { return m_position; }
    qint64 duration() const { return m_duration; }
    bool seekable() const { return m_seekable; }
    qreal volume() const { return m_volume; }
    void setVolume(qreal volume);
    int trackIndex() const { return m_trackIndex; }
    int trackCount() const { return int(m_playlist.size()); }
    QString title() const { return m_title; }
    QString artist() const { return m_artist; }
    QString statusText() const { return m_statusText; }
    QString errorText() const { return m_errorText; }

    Q_INVOKABLE void loadPlaylist(const QStringList& inputs);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE void seek(qint64 deltaMs);
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void adjustVolume(qreal delta);
    Q_INVOKABLE void useDemo();
    Q_INVOKABLE void useMicrophone();
    Q_INVOKABLE void useFile();
    Q_INVOKABLE void stepScopeLowPass(int direction);   // -1 = lower cutoff (more filtering), +1 = higher / off
    Q_INVOKABLE static QString formatTime(qint64 ms);

signals:
    void sourceChanged();
    void playingChanged();
    void hasInputDeviceChanged();
    void levelsChanged();
    void scopeLowPassChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void trackChanged();
    void statusChanged();

private:
    void tick();
    void setPlaying(bool playing);
    void setSource(Source source);
    void setStatus(const QString& status);
    void setError(const QString& error);
    void clearError();
    void resetLevels();
    void updateMonoAndCorrelation();
    void stopSources();
    void loadTrack(int index);
    void setPosition(qint64 position);
    void setDuration(qint64 duration, bool seekable);

    cyber::SpectrumAnalyzer m_analyzer;
    cyber::DemoSynth m_synth;
    QTimer m_timer;

    Source m_source = Source::Idle;
    bool m_playing = false;
    bool m_hasInputDevice = false;

    std::array<float, cyber::kBandCount> m_levels{};
    std::array<float, cyber::kBandCount> m_peaks{};
    std::array<float, cyber::kScopePoints> m_scopeLeft{};
    std::array<float, cyber::kScopePoints> m_scopeRight{};
    float m_vuLeft = 0.f;
    float m_vuRight = 0.f;
    float m_beatPulse = 0.f;
    QVariantList m_bandLevels;
    QVariantList m_peakLevels;
    QList<double> m_waveformLeft;
    QList<double> m_waveformRight;
    QList<double> m_waveformMono;
    float m_correlation = 1.f;

    QList<QUrl> m_playlist;
    int m_trackIndex = -1;
    int m_failedInARow = 0;
    QString m_title;
    QString m_artist;
    QString m_statusText;
    QString m_errorText;
    qint64 m_position = 0;
    qint64 m_duration = 0;
    bool m_seekable = false;
    qreal m_volume = 0.8;

#if CYBER_AUDIO_HAVE_MULTIMEDIA
    void ensurePlayer();
    void ingestBuffer(const QAudioBuffer& buffer);
    void onMicReadyRead();
    void onMediaStatus(QMediaPlayer::MediaStatus status);
    void onPlayerError(QMediaPlayer::Error error, const QString& message);
    void onMetaData();
    void refreshInputDevice();

    QMediaPlayer* m_player = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    QAudioBufferOutput* m_bufferOutput = nullptr;
    QAudioSource* m_micSource = nullptr;
    QIODevice* m_micDevice = nullptr;
    QMediaDevices* m_devices = nullptr;
    QAudioFormat m_micFormat;
    std::vector<float> m_convert;
    int m_buffersForTrack = 0;
#endif

    // Decoded PCM waits here and is fed to the analyzer at real-time pace, one tick's
    // worth per tick, so bursts of large decoder buffers do not make the display flicker.
    void enqueuePcm(const float* interleavedStereo, int frames);
    std::vector<float> m_pending;          // interleaved stereo
    size_t m_pendingRead = 0;
    int m_staleTicks = 0;
    std::array<float, cyber::kBandCount> m_lastTargets{};
    float m_lastVuLeft = 0.f;
    float m_lastVuRight = 0.f;
};
