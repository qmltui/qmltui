# Qamui Branding & Terminal Logo Showcase

Interactive terminal application rendering the complete Qamui brand logo family (`docs/assets/branding/qamui_*`) inside the terminal.

## Overview

This example demonstrates two primary ways Qamui renders logos in terminal applications:

1. **Dynamic Vector & Raster Image Rendering (`Qamui.Image`)**:
   - Vector SVGs (`qamui_mark.svg`, `qamui_lockup_dark.svg`, `qamui_mark_tile.svg`, `qamui_mark_light.svg`, `qamui_lockup_light.svg`, `qamui_mark_mono.svg`)
   - Raster PNG sheets (`qamui_logo_sheet.png`, `qamui_mark_cells.png`)
   - Interactive render modes:
     - `Auto`: Automatically selects Kitty or Sixel graphics protocol if the terminal supports it; otherwise falls back to `HalfBlock`.
     - `HalfBlock`: 1x2 TrueColor subpixels per cell using `▀` (upper half block).
     - `Braille`: 2x4 dot subpixel matrix using Unicode Braille Patterns (`U+2800`..`U+28FF`).
     - `ASCII`: Brightness gradient mapped to standard ASCII characters.
     - `Sixel` / `Kitty`: True pixel graphic protocols.

2. **Native Pre-Rendered ANSI & UTF-8 Text Cells (`Qamui.Text`)**:
   - Direct 24-bit TrueColor SGR escape sequences (`qamui_mark_cells.txt`):
     - `24x12` half-blocks (24x24 effective subpixel resolution)
     - `16x8` half-blocks
     - `10x5` half-blocks
   - Unicode Braille glyph patterns (`20x10`, `12x6`, `8x4`)
   - The braille Q mark and CLI startup banner (`mascot_ascii.txt`)

## Keyboard Controls

| Key | Action |
| --- | --- |
| `[1]` – `[9]` | Quick jump to asset 1 through 9 |
| `[↑]` / `[k]` | Select previous asset |
| `[↓]` / `[j]` | Select next asset |
| `[M]` | Cycle render mode (`Auto` → `HalfBlock` → `Braille` → `ASCII` → `Sixel` → `Kitty`) |
| `[T]` | Toggle background theme (Dark `#0B1020` vs Light `#F6F7F9`) |
| `[S]` | Toggle smooth pixel sampling (`ON` / `OFF`) |
| `[Q]` / `[Esc]` | Quit application |

## Running

```bash
# Build and run directly
cmake --build build --target qamui_branding_logos_demo
./build/examples/topics/branding_logos/qamui_branding_logos_demo

# Pass an initial asset index (e.g. 6 for ANSI half-blocks)
./build/examples/topics/branding_logos/qamui_branding_logos_demo 6
```
