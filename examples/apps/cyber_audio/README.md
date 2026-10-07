# Cyberpunk Audio & Spectrum Visualizer (`cyber_audio`)

A high-performance, real-time procedural synthwave audio & spectrum visualizer engineered to showcase Qtui's complete animation, state machine, and vector rendering capabilities at 60 FPS in the terminal.

## Highlights

- **24-Band Neon Spectrum Analyzer:** Multi-level Unicode blocks (` `, `▂`, `▃`, `▄`, `▅`, `▆`, `▇`, `█`) with decaying peak-hold caps (`▔`) simulated with gravitational decay.
- **Stereo VU Meters:** Analog ballistic damping powered by `Behavior on value` with `Easing.OutQuad` smoothing.
- **High-Resolution Braille Oscilloscope:** Built using Qtui's vector `Canvas` in `RenderMode.Braille` (2x4 subpixels per cell) supporting both real-time audio waveforms and 3D Lissajous phase-scope plots.
- **Dynamic State Morphing:** Four distinct UI layouts (`Split`, `Spectrum Focus`, `Scope Focus`, `Glitch Overdrive`) driven by QML `states` and `transitions` with `Easing.OutBounce` physics.
- **Procedural Synthwave Engine:** Built-in 138 BPM synthwave rhythm generator (Kick, Snare, Hi-Hats, Melodic Arp sweeps) with zero external audio hardware or DSP library dependencies.

## Keyboard Controls

| Key | Action |
| :--- | :--- |
| `[Space]` | Toggle audio synthesis playback / pause |
| `[1]` | Split mode (Spectrum top, Scope + VU bottom) |
| `[2]` | Spectrum Focus mode (Full-height 24-band analyzer) |
| `[3]` | Scope Focus mode (Full-screen Lissajous phase scope) |
| `[B]` | Toggle Bass Boost (+55% low-end frequency boost) |
| `[G]` | Toggle Glitch Overdrive (Emergency red alert mode) |
| `[Q]` / `[Esc]` | Quit application |

## Animation Architecture

```qml
// 1. Ballistic Damping via Behavior
Behavior on vuLeft {
    NumberAnimation { duration: 90; easing: Easing.OutQuad }
}

// 2. Layout Morphing via States & Transitions
transitions: [
    Transition {
        NumberAnimation {
            target: spectrumBox
            property: "height"
            duration: 380
            easing: Easing.OutBounce
        }
    }
]

// 3. High-res Vector Canvas
Canvas {
    renderMode: Canvas.RenderMode.Braille
}
```
