# QmlTui Branding

The QmlTui mark is a geometric **Q** composed of an **O** ring and a `>` terminal prompt chevron whose upper arm crosses the ring to form the tail, paired with a monoline `mlTui` wordmark using a block cursor as the tittle of the `i`.

## Color Palette

| Token | Hex | Usage |
| :--- | :--- | :--- |
| `ACCENT` | `#00FF66` | CRT phosphor green chevron & cursor tittle on dark backgrounds |
| `ACCENT_LIGHT` | `#00A651` | Readable green accent on light backgrounds |
| `INK_DARK_BG` | `#F6F7F9` | Ring and wordmark ink on dark backgrounds |
| `INK_LIGHT_BG` | `#1A1D26` | Ring and wordmark ink on light backgrounds |
| `TILE` | `#0B1020` | Rounded dark app-icon tile background |

## Asset Inventory

| File | Description |
| :--- | :--- |
| `qmltui_lockup_dark.svg` | Horizontal lockup (`QmlTui`) for dark backgrounds (repository & docs site) |
| `qmltui_lockup_light.svg` | Horizontal lockup (`QmlTui`) for light backgrounds |
| `qmltui_mark.svg` | 256×256 color mark for dark backgrounds (transparent) |
| `qmltui_mark_light.svg` | 256×256 color mark for light backgrounds (transparent) |
| `qmltui_mark_mono.svg` | Single-color mark inheriting CSS `currentColor` |
| `qmltui_mark_tile.svg` | 256×256 mark on a rounded `#0B1020` dark app-icon tile |
| `qmltui_logo_sheet.png` | Combined preview contact sheet |
| `qmltui_mark_cells.png` / `qmltui_mark_cells.txt` | Character-cell rendered mark variants |
| `mascot_ascii.txt` | Plain ASCII terminal mark |

## Regenerating SVG Assets

Regenerate all six SVG marks and lockups using [qmltui_logo.py](../../../scripts/branding/qmltui_logo.py) (Python stdlib only):

```bash
python3 scripts/branding/qmltui_logo.py docs/assets/branding
```
