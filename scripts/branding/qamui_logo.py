#!/usr/bin/env python3
"""Generate the Qamui logo family as SVG. Stdlib only.

The mark is a Q built from two glyphs: an O and a `>` prompt whose upper arm
crosses the ring to become the tail. Ring and chevron share one stroke weight.

Outputs (in the directory given as argv[1], default cwd):
  qamui_mark.svg            colour mark, transparent background
  qamui_mark_mono.svg       single-colour mark via currentColor
  qamui_mark_tile.svg       mark on a rounded dark tile (app icon)
  qamui_lockup_dark.svg     the mark as the Q of "Qamui", for dark backgrounds
  qamui_lockup_light.svg    same for light backgrounds
"""
import math, os, sys

ACCENT = "#00FF66"        # CRT phosphor green, from the branding README
ACCENT_LIGHT = "#00A651"  # readable version of the accent on white
INK_DARK_BG = "#F6F7F9"   # ring colour on dark backgrounds
INK_LIGHT_BG = "#1A1D26"  # ring colour on light backgrounds
TILE = "#0B1020"

SW = 26          # stroke of ring and chevron in the 256 box
CX, CY, R = 110, 104, 72
VERTEX_R = 95    # distance of the chevron vertex from the ring centre, along 45 degrees
ARM = 58         # chevron arm length; opening is 90 degrees

def polar(r, deg):
    a = math.radians(deg)
    return CX + r * math.cos(a), CY + r * math.sin(a)

def mark_body(ink, accent):
    vx, vy = polar(VERTEX_R, 45)
    w = ARM * math.cos(math.radians(45))
    pts = f"{vx-w:.1f},{vy-w:.1f} {vx:.1f},{vy:.1f} {vx-w:.1f},{vy+w:.1f}"
    return (f'<circle cx="{CX}" cy="{CY}" r="{R}" fill="none" stroke="{ink}" stroke-width="{SW}"/>\n  '
            f'<polyline points="{pts}" fill="none" stroke="{accent}" stroke-width="{SW}" '
            f'stroke-linecap="round" stroke-linejoin="round"/>')

def svg(w, h, body, title, desc):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" role="img" aria-label="{title}">\n'
            f'  <title>{title}</title>\n  <desc>{desc}</desc>\n  {body}\n</svg>\n')

# ---- wordmark: geometric monoline letters ------------------------------------
# per letter: baseline y=120, x-height top y=40, stroke 18; stroke centres run
# 9..71 horizontally (outer width 80) and 49..111 vertically.
STROKE = 18

def letter(ch, x0, ink, accent):
    g = (f'<g transform="translate({x0} 0)" fill="none" stroke="{ink}" stroke-width="{STROKE}" '
         f'stroke-linecap="round" stroke-linejoin="round">')
    if ch == "a":
        g += '<circle cx="40" cy="80" r="31"/><line x1="71" y1="49" x2="71" y2="111"/>'; adv = 96
    elif ch == "m":
        g += '<path d="M9,111 V64.5 A15.5,15.5 0 0 1 40,64.5 V111 M40,64.5 A15.5,15.5 0 0 1 71,64.5 V111"/>'; adv = 96
    elif ch == "u":
        g += '<path d="M9,49 V80 A31,31 0 0 0 71,80 V49 M71,80 V111"/>'; adv = 96
    elif ch == "i":
        g += '<line x1="9" y1="49" x2="9" y2="111"/>'
        g += f'<rect x="0" y="14" width="18" height="18" fill="{accent}" stroke="none"/>'  # block cursor as the tittle
        adv = 40
    else:
        raise ValueError(ch)
    return g + "</g>", adv

def word(text, x, ink, accent):
    out = []
    for ch in text:
        g, adv = letter(ch, x, ink, accent)
        out.append(g); x += adv
    return "\n  ".join(out), x

def lockup(ink, accent):
    # scale the mark so its stroke matches the letters (18/26), ring bottom on the baseline
    s = STROKE / SW
    ty = 120 - (CY + R + SW / 2) * s
    tx = 10 - (CX - R - SW / 2) * s
    mark = f'<g transform="translate({tx:.1f} {ty:.1f}) scale({s:.4f})">\n  {mark_body(ink, accent)}\n  </g>'
    right = (polar(VERTEX_R, 45)[0] + SW / 2) * s + tx
    w, end = word("amui", round(right + 12), ink, accent)
    return f"{mark}\n  {w}", end + 10

def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    W = lambda name, s: open(os.path.join(out, name), "w").write(s)
    desc = "A Q made of two glyphs: an O and a > prompt whose upper arm crosses the ring to form the tail."
    W("qamui_mark.svg", svg(256, 256, mark_body(INK_DARK_BG, ACCENT), "Qamui mark", desc))
    W("qamui_mark_light.svg", svg(256, 256, mark_body(INK_LIGHT_BG, ACCENT_LIGHT), "Qamui mark for light backgrounds", desc))
    W("qamui_mark_mono.svg", svg(256, 256, mark_body("currentColor", "currentColor"), "Qamui mark, single colour", desc + " Follows currentColor."))
    tile = f'<rect width="256" height="256" rx="56" fill="{TILE}"/>\n  <g transform="translate(26 26) scale(0.8)">\n  {mark_body(INK_DARK_BG, ACCENT)}\n  </g>'
    W("qamui_mark_tile.svg", svg(256, 256, tile, "Qamui app icon", desc))
    for name, ink, accent in (("dark", INK_DARK_BG, ACCENT), ("light", INK_LIGHT_BG, ACCENT_LIGHT)):
        body, width = lockup(ink, accent)
        W(f"qamui_lockup_{name}.svg", svg(width, 170, body, "Qamui", "The Qamui mark standing as the Q of the Qamui wordmark."))
    print("wrote", sorted(f for f in os.listdir(out) if f.startswith("qamui_")))

if __name__ == "__main__":
    main()
