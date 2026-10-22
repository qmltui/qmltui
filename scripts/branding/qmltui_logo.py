#!/usr/bin/env python3
"""Generate the QmlTui logo family as SVG. Stdlib only.

The mark is a Q built from two glyphs: an O and a `>` prompt whose upper arm
crosses the ring to become the tail. Ring and chevron share one stroke weight.

Outputs (in the directory given as argv[1], default cwd):
  qmltui_mark.svg            colour mark for dark backgrounds, transparent background
  qmltui_mark_light.svg      colour mark for light backgrounds, transparent background
  qmltui_mark_mono.svg       single-colour mark via currentColor
  qmltui_mark_tile.svg       mark on a rounded dark tile (app icon)
  qmltui_lockup_dark.svg     the mark as the Q of "QmlTui", for dark backgrounds
  qmltui_lockup_light.svg    same for light backgrounds
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

ACCENT = "#00FF66"        # CRT phosphor green, from the branding README
ACCENT_LIGHT = "#00A651"  # readable version of the accent on white
INK_DARK_BG = "#F6F7F9"   # ring colour on dark backgrounds
INK_LIGHT_BG = "#1A1D26"  # ring colour on light backgrounds
TILE = "#0B1020"

SW = 26          # stroke of ring and chevron in the 256 box
CX, CY, R = 110, 104, 72
VERTEX_R = 95    # distance of the chevron vertex from the ring centre, along 45 degrees
ARM = 58         # chevron arm length; opening is 90 degrees


def polar(r: float, deg: float) -> tuple[float, float]:
    a = math.radians(deg)
    return CX + r * math.cos(a), CY + r * math.sin(a)


def mark_body(ink: str, accent: str) -> str:
    vx, vy = polar(VERTEX_R, 45)
    w = ARM * math.cos(math.radians(45))
    pts = f"{vx - w:.1f},{vy - w:.1f} {vx:.1f},{vy:.1f} {vx - w:.1f},{vy + w:.1f}"
    return (
        f'<circle cx="{CX}" cy="{CY}" r="{R}" fill="none" stroke="{ink}" stroke-width="{SW}"/>\n  '
        f'<polyline points="{pts}" fill="none" stroke="{accent}" stroke-width="{SW}" '
        f'stroke-linecap="round" stroke-linejoin="round"/>'
    )


def svg(w: int, h: int, body: str, title: str, desc: str) -> str:
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" role="img" aria-label="{title}">\n'
        f'  <title>{title}</title>\n  <desc>{desc}</desc>\n  {body}\n</svg>\n'
    )


# ---- wordmark: geometric monoline letters ------------------------------------
# per letter: baseline y=120, x-height top y=40, stroke 18; stroke centres run
# 9..71 horizontally (outer width 80) and 49..111 vertically.
STROKE = 18


def letter(ch: str, x0: int, ink: str, accent: str) -> tuple[str, int]:
    g = (
        f'<g transform="translate({x0} 0)" fill="none" stroke="{ink}" stroke-width="{STROKE}" '
        f'stroke-linecap="round" stroke-linejoin="round">'
    )
    if ch == "a":
        g += '<circle cx="40" cy="80" r="31"/><line x1="71" y1="49" x2="71" y2="111"/>'
        adv = 96
    elif ch == "m":
        g += '<path d="M9,111 V64.5 A15.5,15.5 0 0 1 40,64.5 V111 M40,64.5 A15.5,15.5 0 0 1 71,64.5 V111"/>'
        adv = 96
    elif ch == "l":
        g += '<line x1="9" y1="19" x2="9" y2="111"/>'
        adv = 36
    elif ch == "T":
        g += '<line x1="9" y1="19" x2="67" y2="19"/><line x1="38" y1="19" x2="38" y2="111"/>'
        adv = 88
    elif ch == "t":
        g += '<line x1="25" y1="25" x2="25" y2="111"/><line x1="9" y1="49" x2="45" y2="49"/>'
        adv = 64
    elif ch == "u":
        g += '<path d="M9,49 V80 A31,31 0 0 0 71,80 V49 M71,80 V111"/>'
        adv = 96
    elif ch == "i":
        g += '<line x1="9" y1="49" x2="9" y2="111"/>'
        g += f'<rect x="0" y="14" width="18" height="18" fill="{accent}" stroke="none"/>'  # block cursor as the tittle
        adv = 40
    else:
        raise ValueError(f"Unsupported wordmark character: {ch!r}")
    return g + "</g>", adv


def word(text: str, x: int, ink: str, accent: str) -> tuple[str, int]:
    out = []
    for ch in text:
        g, adv = letter(ch, x, ink, accent)
        out.append(g)
        x += adv
    return "\n  ".join(out), x


def lockup(ink: str, accent: str) -> tuple[str, int]:
    # scale the mark so its stroke matches the letters (18/26), ring bottom on the baseline
    s = STROKE / SW
    ty = 120 - (CY + R + SW / 2) * s
    tx = 10 - (CX - R - SW / 2) * s
    mark = f'<g transform="translate({tx:.1f} {ty:.1f}) scale({s:.4f})">\n  {mark_body(ink, accent)}\n  </g>'
    right = (polar(VERTEX_R, 45)[0] + SW / 2) * s + tx
    w, end = word("mlTui", round(right + 12), ink, accent)
    return f"{mark}\n  {w}", end + 10


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate the QmlTui SVG logo family.")
    parser.add_argument(
        "output_dir",
        nargs="?",
        default=Path("."),
        type=Path,
        help="Output directory for generated SVG files (default: current directory)",
    )
    args = parser.parse_args()

    out_dir: Path = args.output_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    written: list[str] = []

    def write_svg(name: str, content: str) -> None:
        (out_dir / name).write_text(content, encoding="utf-8")
        written.append(name)

    desc = "A Q made of two glyphs: an O and a > prompt whose upper arm crosses the ring to form the tail."
    write_svg("qmltui_mark.svg", svg(256, 256, mark_body(INK_DARK_BG, ACCENT), "QmlTui mark", desc))
    write_svg("qmltui_mark_light.svg", svg(256, 256, mark_body(INK_LIGHT_BG, ACCENT_LIGHT), "QmlTui mark for light backgrounds", desc))
    write_svg("qmltui_mark_mono.svg", svg(256, 256, mark_body("currentColor", "currentColor"), "QmlTui mark, single colour", desc + " Follows currentColor."))
    tile = (
        f'<rect width="256" height="256" rx="56" fill="{TILE}"/>\n'
        f'  <g transform="translate(26 26) scale(0.8)">\n  {mark_body(INK_DARK_BG, ACCENT)}\n  </g>'
    )
    write_svg("qmltui_mark_tile.svg", svg(256, 256, tile, "QmlTui app icon", desc))
    for name, ink, accent in (("dark", INK_DARK_BG, ACCENT), ("light", INK_LIGHT_BG, ACCENT_LIGHT)):
        body, width = lockup(ink, accent)
        write_svg(
            f"qmltui_lockup_{name}.svg",
            svg(width, 170, body, "QmlTui", "The QmlTui mark standing as the Q of the QmlTui wordmark."),
        )
    print("wrote", sorted(written))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
