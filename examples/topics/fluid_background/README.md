# Fluid Background Showcase (Asciify-inspired)

An implementation of procedural domain-warping fluid dynamics rendered as an ASCII/Unicode character field in QmlTui, inspired by [Asciify Fluid Background](https://asciify.org/docs/backgrounds/fluid).

## Features

- **Procedural Harmonic Fluid Equations**: Implements multi-frequency sine wave domain warping, producing organic folding ribbons, ripples, and continuous flow fields across the terminal grid.
- **Interactive Mouse Wakes**: Clicking or dragging the mouse excites decaying radial wave packets that deform the fluid surface.
- **Dynamic ASCII/Unicode Charset Ramps**:
  - Standard ASCII (` ·:;=+*#%@`)
  - Fine Dither (` ·∙•○◍●`)
  - Terminal Blocks (` ░▒▓█`)
  - Unicode Braille (` ⠁⠃⠇⠧⠷⠿`)
  - Binary Stream (` 01`)
- **Color Modes**:
  - Two-Color Linear Gradient (e.g. Sky Cyan to Hot Pink)
  - Accent with deep ambient background
  - Grayscale high-contrast
- **Performance**: Native C++ cell grid evaluation executes in `< 0.1ms` per frame, running at a smooth 25 FPS without display server or GPU rasterization dependencies.

## Key Bindings

- `[Space]` or `[P]`: Play / Pause animation
- `[S]`: Cycle character set
- `[C]`: Cycle color palette & mode
- `[W]`: Inject a random wake
- `[Mouse Click / Drag]`: Excite physical ripples at cursor location
- `[Q]` or `[ESC]`: Quit
