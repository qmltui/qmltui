# Cyberpunk Audio Visualizer & Player (`cyber_audio`)

A terminal spectrum visualizer and audio player built on Qtui. It analyzes real audio
with an FFT in C++ and renders a 24-band neon equalizer, stereo VU meters and a Braille
oscilloscope that fill the whole terminal. Three sources feed the same display:

| Source | Badge | How | Requires |
| :--- | :--- | :--- | :--- |
| Media files / URLs | `FILE` | `QMediaPlayer` + `QAudioOutput` for playback, `QAudioBufferOutput` for the decoded PCM | Qt Multimedia with the FFmpeg backend |
| Microphone | `MIC` | `QAudioSource` on the default input device | Qt Multimedia |
| Procedural demo | `DEMO` | Deterministic 138 BPM synthwave generator in C++, only via `--demo` (showcase and automated tests) | nothing |

Without media on the command line the app starts idle and the transport row explains
how to start. Qt Multimedia is optional at build time. When CMake cannot find it, the
example still builds; file and microphone requests explain that they are unavailable and
`--demo` runs the showcase. Look for `cyber_audio: Qt Multimedia found` or `... not found` in the
configure output.

## Usage

```bash
qtui_cyber_audio [--mic | --demo] [--volume <0-100>] [media...]

qtui_cyber_audio ~/Music/album/            # play a directory (recursive, sorted)
qtui_cyber_audio track.flac other.mp3      # explicit playlist
qtui_cyber_audio https://host/stream.mp3   # URLs are passed to QMediaPlayer
qtui_cyber_audio --mic                     # live microphone visualizer
qtui_cyber_audio --demo                    # procedural showcase, no audio needed
qtui_cyber_audio                           # idle: press M or restart with media
```

Directories are scanned for `mp3 flac wav ogg oga opus m4a aac wma aiff aif`. Files
given explicitly are always kept. Tracks auto-advance at the end and wrap around.

## Keyboard Controls

The key map is always visible in the footer; keys that do not apply to the current
source are hidden. Visible panels and the active source are highlighted.

| Key | Action |
| :--- | :--- |
| `Space` | Pause / resume (files pause the player, the microphone suspends capture) |
| `←` / `→` | Seek 5 s backward / forward (file source) |
| `N` / `P` | Next / previous track (also returns from MIC or DEMO to the playlist) |
| `+` / `-` | Volume in 5 % steps (file source) |
| `1` / `2` / `3` | Toggle the equalizer, the oscilloscope and the VU meters; the remaining panels grow into the freed space |
| `V` | Cycle the oscilloscope art: stereo traces → phosphor persistence → ridge plot → neon nebula |
| `[` / `]` | Scope low-pass filter: `[` lowers the cutoff (off → 8 kHz → 4 kHz → 2 kHz → 1 kHz → 600 → 300 → 150 Hz), `]` raises it back to off |
| `F` | Toggle immersive Neon Nebula; Escape returns to the previous dashboard |
| `A` | Blend: where lines share a cell, average their colors instead of letting the top one win |
| `M` | Switch to the microphone |
| `Q` / `Esc` | Quit (Escape leaves immersive art first) |

## What is on screen

- **Header:** source badge, `TRACK n/m`, title and artist from the media metadata
  (file name when tags are missing), play state and volume.
- **Transport row:** elapsed time, seek bar and duration for files; a status line for
  the demo and microphone; errors (missing file, unsupported format, no microphone) in
  red in the same row.
- **Equalizer:** 24 log-spaced bands from 40 Hz to 18 kHz, rendered at 1/8-cell
  resolution with attack/release smoothing and peak-hold caps. Bar width adapts to the
  terminal; labels thin out when bars get narrow.
- **Stereo VU:** RMS per channel with a full-scale sine reading 0 dB.
- **Oscilloscope:** `Canvas` in `RenderMode.Braille` (2x4 subpixels per cell) showing
  the real stereo waveform, left and right overlaid in the colors of the VU captions.
  Each channel is a `WaveformTrace`, a small C++ `CanvasShape` that plots a sample array
  as a polyline, so no JavaScript runs per dot. `V` swaps in other C++ shapes from
  `ScopeArt.h`: `PersistenceTrace` keeps the last eight windows and fades them like a
  CRT phosphor, and `RidgePlot` stacks the last 24 mono windows back to front with
  hidden-line removal in the style of the "Unknown Pleasures" cover.
  The window is trigger-aligned to a rising zero crossing so periodic content holds
  still. An optional second-order Butterworth low-pass on the display path (`[` / `]`,
  off by default) removes high-frequency detail so the bass and low-mid shape stands out;
  the panel title shows the current cutoff. The FFT for the equalizer always uses the
  unfiltered signal.

### Neon Nebula

Press **F** for a full-area cyan and magenta ribbon field, or cycle **V** to
Nebula inside the scope panel. Bass expands the curling arms, mids bend their
shape, and treble brightens drifting particles. Changes relative to the recent
spectrum deform individual sections of the arms; audio attacks expand the field
and accelerate its flow, including during file and microphone playback. Quiet
passages settle and Space freezes/resumes the artwork. F or Escape restores the previous dashboard layout.
The scope low-pass filter does not affect Nebula: it uses the unfiltered spectrum.

`NeonNebula` in `ScopeArt.h/.cpp` draws directly into the Braille canvas. Each
audio-engine update (about 33 ms) advances the visual only while it is visible and playback is active. A slow spectral baseline exposes changes in
compressed music, and a short positive spectral-flux envelope highlights attacks.
Drawing uses deterministic particles and bounded work, with no accumulated frame history or per-dot JavaScript.

## Architecture

```
main.cpp / main_test.cpp ──cliOptions──▶ Main.qml ──▶ AudioEngine (QML_ELEMENT)
                                                        ├─ DemoSynth        (C++ port of the synth)
                                                        ├─ QMediaPlayer + QAudioBufferOutput
                                                        └─ QAudioSource
                                                               │ float PCM, queued
                                                        SpectrumAnalyzer: 2048-pt Hann + radix-2 FFT
                                                        → 24 log bands (dB → 0..1), RMS L/R, scope
```

- The QML is split into small components inside the `CyberAudio` module. `Main.qml`
  owns the engine, key map and widget toggles and composes `HeaderBar`, `TransportBar`,
  `SpectrumPanel`, `ScopePanel` and `KeyMapBar`. `ScopePanel` reuses `VuMeter` and
  the `WaveformTrace` shape; `KeyMapBar` reuses `KeyHint`. Colors and text helpers live in the
  `Theme` singleton. App-specific components take the engine as a `required property`;
  `KeyHint`, `VuMeter`, `WaveformTrace` and `Theme` do not know about it and can be
  copied into other Qtui projects.
- `SpectrumAnalyzer` (`SpectrumAnalyzer.h/.cpp`) is plain C++ with no Qt Multimedia
  dependency, so it is unit tested directly.
- `AudioEngine` ticks every 33 ms. Decoded PCM is queued and consumed one tick's worth
  at a time, so the picture follows the sound at real-time pace even though the decoder
  delivers large buffers in bursts. All smoothing lives here; QML only renders, and every
  level property notifies through a single `levelsChanged()` per tick.
- Multimedia code is compiled only when `CYBER_AUDIO_HAVE_MULTIMEDIA` is 1. Backends
  are created lazily on first use, so demo runs never touch the audio system.
- The example does not use `Behavior`, so it builds with `QTUI_ENABLE_BEHAVIOR` on or
  off. Toggling a panel animates the others into the freed space through the
  `spectrumShare` transition; accent colors
  switch instantly.
- The production binary does not link `Qtui::Runner` and starts idle without media;
  `qtui_cyber_audio_test` links it and defaults to the demo source so scenarios are
  deterministic (`--idle` opts out).

## Building and testing

```bash
cmake -S . -B build -DQTUI_BUILD_TESTS=ON -DQTUI_BUILD_EXAMPLES=ON
cmake --build build --target qtui_cyber_audio qtui_cyber_audio_test test_cyber_audio_dsp
ctest --test-dir build -R 'cyber_audio' --output-on-failure
```

- `cyber_audio_dsp`: Qt Test for band geometry, FFT, tone detection, RMS, the demo
  synth, playlist scanning and the engine's demo/pause behaviour.
- `cyber_audio_demo_smoke`: Runner scenario against committed frames in demo mode.

To inspect real playback without a terminal session, generate a WAV and run the test
binary with `QTUI_RUNNER_EVAL`:

```bash
QTUI_RUNNER_EVAL="terminal.waitForText('FILE'); terminal.sleep(1500); terminal.captureText('play.txt');" \
QTUI_RUNNER_COLUMNS=120 QTUI_RUNNER_ROWS=36 QTUI_RUNNER_ARTIFACT_DIR=/tmp/cyber \
./build/examples/apps/cyber_audio/qtui_cyber_audio_test tone.wav
```

## Troubleshooting

- **No sound but the bars move:** the analysis reads the decoded PCM independently of
  the output device. Check the default audio output (PulseAudio / PipeWire).
- **Flat bars and "No audio data received":** either the audio device is held by another
  player (a second `qtui_cyber_audio` instance under WSLg receives no decoded PCM until the
  first one quits), or the Qt Multimedia FFmpeg backend is missing; `QAudioBufferOutput` is
  served only by that backend.
- **Qt Multimedia warnings after exit:** Qtui buffers Qt log messages while the
  alternate screen is active and prints them when the application ends.
