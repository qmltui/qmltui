# Cyberpunk Audio & Spectrum Visualizer (`cyber_audio`)

A real-time procedural synthwave spectrum visualizer that showcases Qtui's
animation, state machine, and Braille vector rendering in the terminal. The
layout is fully responsive: the header and key map take one row each, and the
two panels split the remaining height by a ratio that the layout modes animate.

## Highlights

- **24-Band Neon Equalizer:** Bars render at 1/8-cell vertical resolution
  (`▁▂▃▄▅▆▇█`) with exponential attack/release smoothing and gravity-driven
  peak-hold caps (`▔`). Bar width and spacing adapt to the terminal width, and
  frequency labels thin out automatically when the bars get narrow.
- **Stereo VU Meters:** Fast-attack / slow-release ballistics rendered with
  `ProgressBar` thresholds (accent → amber → red).
- **Braille Oscilloscope:** Qtui `Canvas` in `RenderMode.Braille` (2x4
  subpixels per cell) drawing either a synthwave waveform or a Lissajous phase
  plot, sized to whatever space the current layout gives it.
- **Layout Morphing:** Three layouts (`Split`, `Spectrum`, `Scope`) driven by
  QML `states` that change a single `spectrumShare` ratio; the `Transition`
  animates it with `Easing.OutBounce`.
- **Procedural Synthwave Engine:** 138 BPM kick / snare / hi-hat / arp
  generator with no audio hardware or DSP dependency.

## Keyboard Controls

The key map is always visible in the footer. The active layout and enabled
effects are highlighted there and shown as badges in the header.

| Key | Group | Action |
| :--- | :--- | :--- |
| `Space` | Transport | Pause / resume the synth engine |
| `1` | Layout | Split: equalizer above, VU meters + oscilloscope below |
| `2` | Layout | Spectrum: tall equalizer, compact scope strip |
| `3` | Layout | Scope: large Lissajous phase scope, compact equalizer |
| `B` | Effect | Toggle bass boost (+55 % low-end energy) |
| `G` | Effect | Toggle glitch overdrive (noise injection, alert colors) |
| `Q` / `Esc` | | Quit |

Layout keys and effect keys are independent: bass boost and glitch stay on
while you switch layouts.

## Architecture Notes

```qml
// 1. One animated ratio drives both panel heights
states: [
    State { name: "split";    PropertyChanges { target: root; properties: { "spectrumShare": 0.55 } } },
    State { name: "spectrum"; PropertyChanges { target: root; properties: { "spectrumShare": 0.78 } } },
    State { name: "scope";    PropertyChanges { target: root; properties: { "spectrumShare": 0.28 } } }
]
transitions: [
    Transition {
        NumberAnimation { target: root; property: "spectrumShare"; duration: 380; easing: Easing.OutBounce }
    }
]

// 2. The equalizer is one Text per row; each row string covers all 24 bands
Repeater {
    model: eqArea.barRows
    delegate: Text {
        text: root.spectrumRow(rowFromBottom, eqArea.barRows, eqArea.barWidth, eqArea.gap, eqArea.leftPad)
    }
}

// 3. High-resolution vector canvas
Canvas {
    renderMode: Canvas.RenderMode.Braille
}
```
