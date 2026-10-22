#!/usr/bin/env python3
"""Render asciicast v2 (.cast) recordings to animated GIF and static SVG files."""

from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import html
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unicodedata


DEFAULT_BG = "#0d1117"
DEFAULT_FG = "#c9d1d9"

HEX_COLOR_RE = re.compile(r"^#[0-9a-fA-F]{6}$")

ANSI_16 = [
    "#0d1117",  # 0 black
    "#f87171",  # 1 red
    "#2cde85",  # 2 green
    "#fbbf24",  # 3 yellow
    "#38bdf8",  # 4 blue
    "#c084fc",  # 5 magenta
    "#7ee0ff",  # 6 cyan
    "#c9d1d9",  # 7 white
    "#8b949e",  # 8 bright black (gray)
    "#ff7b72",  # 9 bright red
    "#00ff66",  # 10 bright green
    "#fde047",  # 11 bright yellow
    "#79c0ff",  # 12 bright blue
    "#d2a8ff",  # 13 bright magenta
    "#a5f3fc",  # 14 bright cyan
    "#f0f6fc",  # 15 bright white
]

# Box-drawing character definitions: (left, right, up, down) where 0=none, 1=single, 2=double, 3=rounded
BOX_DRAWING: dict[str, tuple[int, int, int, int]] = {
    "─": (1, 1, 0, 0),
    "━": (2, 2, 0, 0),
    "│": (0, 0, 1, 1),
    "┃": (0, 0, 2, 2),
    "┌": (0, 1, 0, 1),
    "┍": (0, 2, 0, 1),
    "┎": (0, 1, 0, 2),
    "┏": (0, 2, 0, 2),
    "┐": (1, 0, 0, 1),
    "┑": (2, 0, 0, 1),
    "┒": (1, 0, 0, 2),
    "┓": (2, 0, 0, 2),
    "└": (0, 1, 1, 0),
    "┕": (0, 2, 1, 0),
    "┖": (0, 1, 2, 0),
    "┗": (0, 2, 2, 0),
    "┘": (1, 0, 1, 0),
    "┙": (2, 0, 1, 0),
    "┚": (1, 0, 2, 0),
    "┛": (2, 0, 2, 0),
    "├": (0, 1, 1, 1),
    "┝": (0, 2, 1, 1),
    "┠": (0, 1, 2, 2),
    "┣": (0, 2, 2, 2),
    "┤": (1, 0, 1, 1),
    "┥": (2, 0, 1, 1),
    "┨": (1, 0, 2, 2),
    "┫": (2, 0, 2, 2),
    "┬": (1, 1, 0, 1),
    "┳": (2, 2, 0, 2),
    "┴": (1, 1, 1, 0),
    "┻": (2, 2, 2, 0),
    "┼": (1, 1, 1, 1),
    "╋": (2, 2, 2, 2),
    "═": (2, 2, 0, 0),
    "║": (0, 0, 2, 2),
    "╔": (0, 2, 0, 2),
    "╗": (2, 0, 0, 2),
    "╚": (0, 2, 2, 0),
    "╝": (2, 0, 2, 0),
    "╠": (0, 2, 2, 2),
    "╣": (2, 0, 2, 2),
    "╦": (2, 2, 0, 2),
    "╩": (2, 2, 2, 0),
    "╬": (2, 2, 2, 2),
    "╭": (0, 1, 0, 1),
    "╮": (1, 0, 0, 1),
    "╯": (1, 0, 1, 0),
    "╰": (0, 1, 1, 0),
    "╴": (1, 0, 0, 0),
    "╵": (0, 0, 1, 0),
    "╶": (0, 1, 0, 0),
    "╷": (0, 0, 0, 1),
}


def xterm_256_color(idx: int) -> str:
    idx = max(0, min(255, idx))
    if idx < 16:
        return ANSI_16[idx]
    if idx < 232:
        n = idx - 16
        r = (n // 36) % 6
        g = (n // 6) % 6
        b = n % 6
        conv = lambda v: 0 if v == 0 else 55 + v * 40
        return f"#{conv(r):02x}{conv(g):02x}{conv(b):02x}"
    gray = 8 + (idx - 232) * 10
    gray = max(0, min(255, gray))
    return f"#{gray:02x}{gray:02x}{gray:02x}"


def char_display_width(ch: str) -> int:
    if not ch:
        return 0
    cp = ord(ch[0])
    if cp < 0x20 or (0x7F <= cp < 0xA0):
        return 0
    if unicodedata.combining(ch[0]):
        return 0
    if cp in (0x200B, 0x200C, 0x200D, 0xFE0E, 0xFE0F):
        return 0
    if 0x2500 <= cp <= 0x2BFF:
        return 1
    eaw = unicodedata.east_asian_width(ch[0])
    if eaw in ("W", "F") or (0x1F300 <= cp <= 0x1FAFF):
        return 2
    return 1


@dataclass(slots=True)
class Cell:
    ch: str = " "
    fg: str = DEFAULT_FG
    bg: str = DEFAULT_BG
    bold: bool = False
    dim: bool = False
    italic: bool = False
    underline: bool = False
    reverse: bool = False
    strikethrough: bool = False
    cont: bool = False

    def copy(self) -> "Cell":
        return Cell(
            self.ch,
            self.fg,
            self.bg,
            self.bold,
            self.dim,
            self.italic,
            self.underline,
            self.reverse,
            self.strikethrough,
            self.cont,
        )

    def key(self) -> tuple:
        return (
            self.ch,
            self.fg,
            self.bg,
            self.bold,
            self.dim,
            self.italic,
            self.underline,
            self.reverse,
            self.strikethrough,
            self.cont,
        )


class TerminalSimulator:
    def __init__(self, width: int, height: int, default_fg: str = DEFAULT_FG, default_bg: str = DEFAULT_BG) -> None:
        self.width = max(1, width)
        self.height = max(1, height)
        self.initial_fg = default_fg
        self.initial_bg = default_bg
        self.default_fg = default_fg
        self.default_bg = default_bg
        self.cx = 0
        self.cy = 0
        self.saved_cx = 0
        self.saved_cy = 0
        self.fg = default_fg
        self.bg = default_bg
        self.bold = False
        self.dim = False
        self.italic = False
        self.underline = False
        self.reverse = False
        self.strikethrough = False
        self.grid: list[list[Cell]] = [
            [self._blank_cell() for _ in range(self.width)] for _ in range(self.height)
        ]

    def _blank_cell(self, bg: str | None = None) -> Cell:
        return Cell(" ", self.default_fg, bg if bg is not None else self.default_bg)

    def snapshot(self) -> list[list[Cell]]:
        return [[cell.copy() for cell in row] for row in self.grid]

    def snapshot_key(self) -> tuple:
        return tuple(tuple(cell.key() for cell in row) for row in self.grid)

    def _scroll_up(self) -> None:
        self.grid.pop(0)
        self.grid.append([self._blank_cell() for _ in range(self.width)])

    def _handle_osc(self, payload: str) -> None:
        if ";" in payload:
            code, value = payload.split(";", 1)
        else:
            code, value = payload, ""
        value = value.strip()
        if code == "10" and HEX_COLOR_RE.match(value):
            self.default_fg = value.lower()
        elif code == "11" and HEX_COLOR_RE.match(value):
            self.default_bg = value.lower()
        elif code == "110":
            self.default_fg = self.initial_fg
        elif code == "111":
            self.default_bg = self.initial_bg

    def feed(self, data: str) -> None:
        i = 0
        n = len(data)
        while i < n:
            ch = data[i]
            if ch == "\x1b":
                if i + 1 >= n:
                    break
                nxt = data[i + 1]
                if nxt == "[":
                    j = i + 2
                    while j < n and not (0x40 <= ord(data[j]) <= 0x7E):
                        j += 1
                    if j < n:
                        params_str = data[i + 2 : j]
                        final_ch = data[j]
                        self._handle_csi(params_str, final_ch)
                        i = j + 1
                        continue
                    break
                if nxt in ("]", "P", "_", "^", "X"):
                    j = i + 2
                    payload_end = j
                    while j < n:
                        if data[j] == "\x07":
                            payload_end = j
                            j += 1
                            break
                        if data[j] == "\x1b" and j + 1 < n and data[j + 1] == "\\":
                            payload_end = j
                            j += 2
                            break
                        j += 1
                    else:
                        payload_end = n
                    if nxt == "]":
                        self._handle_osc(data[i + 2 : payload_end])
                    i = j
                    continue
                if 0x20 <= ord(nxt) <= 0x2F:
                    j = i + 2
                    while j < n and 0x20 <= ord(data[j]) <= 0x2F:
                        j += 1
                    if j < n:
                        j += 1
                    i = j
                    continue
                i += 2
                continue

            if ch == "\r":
                self.cx = 0
                i += 1
                continue
            if ch == "\n":
                self.cy += 1
                if self.cy >= self.height:
                    self._scroll_up()
                    self.cy = self.height - 1
                i += 1
                continue
            if ch == "\b":
                self.cx = max(0, self.cx - 1)
                i += 1
                continue
            if ch == "\t":
                self.cx = min(self.width - 1, (self.cx // 8 + 1) * 8)
                i += 1
                continue
            if ord(ch) < 0x20:
                i += 1
                continue

            w = char_display_width(ch)
            if w == 0:
                i += 1
                continue

            if self.cx >= self.width or (w == 2 and self.cx + 1 >= self.width and self.width > 1):
                if self.cx < self.width and 0 <= self.cy < self.height:
                    self.grid[self.cy][self.cx] = self._blank_cell(self.bg)
                self.cx = 0
                self.cy += 1
                if self.cy >= self.height:
                    self._scroll_up()
                    self.cy = self.height - 1

            if 0 <= self.cy < self.height and 0 <= self.cx < self.width:
                existing = self.grid[self.cy][self.cx]
                if existing.cont and self.cx > 0:
                    self.grid[self.cy][self.cx - 1] = self._blank_cell(self.bg)
                if self.cx + 1 < self.width and self.grid[self.cy][self.cx + 1].cont:
                    self.grid[self.cy][self.cx + 1] = self._blank_cell(self.bg)

                self.grid[self.cy][self.cx] = Cell(
                    ch=ch,
                    fg=self.fg,
                    bg=self.bg,
                    bold=self.bold,
                    dim=self.dim,
                    italic=self.italic,
                    underline=self.underline,
                    reverse=self.reverse,
                    strikethrough=self.strikethrough,
                    cont=False,
                )
                if w == 2 and self.cx + 1 < self.width:
                    if self.cx + 2 < self.width and self.grid[self.cy][self.cx + 2].cont:
                        self.grid[self.cy][self.cx + 2] = self._blank_cell(self.bg)
                    self.grid[self.cy][self.cx + 1] = Cell(
                        ch="",
                        fg=self.fg,
                        bg=self.bg,
                        bold=self.bold,
                        dim=self.dim,
                        italic=self.italic,
                        underline=self.underline,
                        reverse=self.reverse,
                        strikethrough=self.strikethrough,
                        cont=True,
                    )
            self.cx = min(self.width, self.cx + w)
            i += 1

    def _handle_csi(self, params_str: str, final_ch: str) -> None:
        if params_str.startswith(("?", ">", "=")):
            return
        parts = params_str.split(";") if params_str else []

        def param(idx: int, default: int) -> int:
            if idx < len(parts) and parts[idx].isdigit():
                val = int(parts[idx])
                return val if val > 0 else default
            return default

        if final_ch in ("H", "f"):
            row = param(0, 1) - 1
            col = param(1, 1) - 1
            self.cy = max(0, min(self.height - 1, row))
            self.cx = max(0, min(self.width - 1, col))
        elif final_ch == "A":
            self.cy = max(0, self.cy - param(0, 1))
        elif final_ch == "B":
            self.cy = min(self.height - 1, self.cy + param(0, 1))
        elif final_ch == "C":
            self.cx = min(self.width - 1, self.cx + param(0, 1))
        elif final_ch == "D":
            self.cx = max(0, self.cx - param(0, 1))
        elif final_ch == "E":
            self.cy = min(self.height - 1, self.cy + param(0, 1))
            self.cx = 0
        elif final_ch == "F":
            self.cy = max(0, self.cy - param(0, 1))
            self.cx = 0
        elif final_ch == "G":
            self.cx = max(0, min(self.width - 1, param(0, 1) - 1))
        elif final_ch == "d":
            self.cy = max(0, min(self.height - 1, param(0, 1) - 1))
        elif final_ch == "s":
            self.saved_cx, self.saved_cy = self.cx, self.cy
        elif final_ch == "u":
            self.cx, self.cy = self.saved_cx, self.saved_cy
        elif final_ch == "J":
            mode = int(parts[0]) if parts and parts[0].isdigit() else 0
            if mode in (2, 3):
                for r in range(self.height):
                    for c in range(self.width):
                        self.grid[r][c] = self._blank_cell(self.bg)
            elif mode == 0:
                for c in range(self.cx, self.width):
                    self.grid[self.cy][c] = self._blank_cell(self.bg)
                for r in range(self.cy + 1, self.height):
                    for c in range(self.width):
                        self.grid[r][c] = self._blank_cell(self.bg)
            elif mode == 1:
                for r in range(0, self.cy):
                    for c in range(self.width):
                        self.grid[r][c] = self._blank_cell(self.bg)
                for c in range(0, min(self.width, self.cx + 1)):
                    self.grid[self.cy][c] = self._blank_cell(self.bg)
        elif final_ch == "K":
            mode = int(parts[0]) if parts and parts[0].isdigit() else 0
            if 0 <= self.cy < self.height:
                if mode == 0:
                    for c in range(min(self.cx, self.width), self.width):
                        self.grid[self.cy][c] = self._blank_cell(self.bg)
                elif mode == 1:
                    for c in range(0, min(self.width, self.cx + 1)):
                        self.grid[self.cy][c] = self._blank_cell(self.bg)
                elif mode == 2:
                    for c in range(self.width):
                        self.grid[self.cy][c] = self._blank_cell(self.bg)
        elif final_ch == "X":
            count = param(0, 1)
            if 0 <= self.cy < self.height:
                for c in range(min(self.cx, self.width), min(self.width, self.cx + count)):
                    self.grid[self.cy][c] = self._blank_cell(self.bg)
        elif final_ch == "m":
            self._handle_sgr(parts)

    def _handle_sgr(self, parts: list[str]) -> None:
        if not parts:
            parts = ["0"]
        nums = [int(p) if p.isdigit() else 0 for p in parts]
        i = 0
        while i < len(nums):
            code = nums[i]
            if code == 0:
                self.fg = self.default_fg
                self.bg = self.default_bg
                self.bold = False
                self.dim = False
                self.italic = False
                self.underline = False
                self.reverse = False
                self.strikethrough = False
            elif code == 1:
                self.bold = True
            elif code == 2:
                self.dim = True
            elif code == 3:
                self.italic = True
            elif code == 4:
                self.underline = True
            elif code == 7:
                self.reverse = True
            elif code == 9:
                self.strikethrough = True
            elif code == 22:
                self.bold = False
                self.dim = False
            elif code == 23:
                self.italic = False
            elif code == 24:
                self.underline = False
            elif code == 27:
                self.reverse = False
            elif code == 29:
                self.strikethrough = False
            elif 30 <= code <= 37:
                self.fg = ANSI_16[code - 30]
            elif code == 39:
                self.fg = self.default_fg
            elif 40 <= code <= 47:
                self.bg = ANSI_16[code - 40]
            elif code == 49:
                self.bg = self.default_bg
            elif 90 <= code <= 97:
                self.fg = ANSI_16[8 + (code - 90)]
            elif 100 <= code <= 107:
                self.bg = ANSI_16[8 + (code - 100)]
            elif code in (38, 48):
                is_fg = code == 38
                if i + 2 < len(nums) and nums[i + 1] == 5:
                    color = xterm_256_color(nums[i + 2])
                    if is_fg:
                        self.fg = color
                    else:
                        self.bg = color
                    i += 2
                elif i + 4 < len(nums) and nums[i + 1] == 2:
                    r = max(0, min(255, nums[i + 2]))
                    g = max(0, min(255, nums[i + 3]))
                    b = max(0, min(255, nums[i + 4]))
                    color = f"#{r:02x}{g:02x}{b:02x}"
                    if is_fg:
                        self.fg = color
                    else:
                        self.bg = color
                    i += 4
            i += 1


def _render_vector_cell_glyph(
    ch: str,
    x: int,
    y: int,
    cell_w: int,
    cell_h: int,
    fg: str,
    dim: bool,
) -> str | None:
    opacity = ' opacity="0.65"' if dim else ""
    # Block elements
    if ch == "█":
        return f'<rect x="{x}" y="{y}" width="{cell_w}" height="{cell_h}" fill="{fg}"{opacity}/>'
    if ch == "▀":
        return f'<rect x="{x}" y="{y}" width="{cell_w}" height="{cell_h / 2}" fill="{fg}"{opacity}/>'
    if ch == "▄":
        return f'<rect x="{x}" y="{y + cell_h / 2}" width="{cell_w}" height="{cell_h / 2}" fill="{fg}"{opacity}/>'
    if ch == "▌":
        return f'<rect x="{x}" y="{y}" width="{cell_w / 2}" height="{cell_h}" fill="{fg}"{opacity}/>'
    if ch == "▐":
        return f'<rect x="{x + cell_w / 2}" y="{y}" width="{cell_w / 2}" height="{cell_h}" fill="{fg}"{opacity}/>'
    if ch in ("░", "▒", "▓"):
        alpha = 0.25 if ch == "░" else (0.5 if ch == "▒" else 0.75)
        if dim:
            alpha *= 0.65
        return f'<rect x="{x}" y="{y}" width="{cell_w}" height="{cell_h}" fill="{fg}" fill-opacity="{alpha:.2f}"/>'
    if ch in ("▁", "▂", "▃", "▄", "▅", "▆", "▇"):
        eighths = "▁▂▃▄▅▆▇".index(ch) + 1
        bh = (cell_h * eighths) / 8.0
        by = y + cell_h - bh
        return f'<rect x="{x}" y="{by:.2f}" width="{cell_w}" height="{bh:.2f}" fill="{fg}"{opacity}/>'
    if ch == "▰":
        bh = cell_h * 0.45
        by = y + (cell_h - bh) / 2.0
        return f'<rect x="{x + 0.5:.1f}" y="{by:.1f}" width="{cell_w - 1:.1f}" height="{bh:.1f}" rx="1" fill="{fg}"{opacity}/>'
    if ch == "▱":
        bh = cell_h * 0.45
        by = y + (cell_h - bh) / 2.0
        return f'<rect x="{x + 1:.1f}" y="{by:.1f}" width="{cell_w - 2:.1f}" height="{bh:.1f}" rx="1" fill="none" stroke="{fg}" stroke-width="1" stroke-opacity="0.55"/>'

    # Box drawing characters (single, double, rounded)
    if ch in BOX_DRAWING:
        left, right, up, down = BOX_DRAWING[ch]
        cx = x + cell_w / 2.0
        cy = y + cell_h / 2.0
        x0, x1 = float(x), float(x + cell_w)
        y0, y1 = float(y), float(y + cell_h)
        parts: list[str] = []
        d = 1.5  # half-separation for double lines
        if ch in ("╔", "╗", "╚", "╝") or (2 in (left, right, up, down) and ch in ("═", "║", "╠", "╣", "╦", "╩", "╬")):
            # Double-line box drawing with clean inner/outer corner joins
            if ch == "═":
                parts.append(f"M{x0},{cy - d} H{x1} M{x0},{cy + d} H{x1}")
            elif ch == "║":
                parts.append(f"M{cx - d},{y0} V{y1} M{cx + d},{y0} V{y1}")
            elif ch == "╔":
                parts.append(f"M{x1},{cy - d} H{cx - d} V{y1} M{x1},{cy + d} H{cx + d} V{y1}")
            elif ch == "╗":
                parts.append(f"M{x0},{cy - d} H{cx + d} V{y1} M{x0},{cy + d} H{cx - d} V{y1}")
            elif ch == "╚":
                parts.append(f"M{x1},{cy + d} H{cx - d} V{y0} M{x1},{cy - d} H{cx + d} V{y0}")
            elif ch == "╝":
                parts.append(f"M{x0},{cy + d} H{cx + d} V{y0} M{x0},{cy - d} H{cx - d} V{y0}")
            else:
                if left:
                    parts.append(f"M{x0},{cy - d} H{cx} M{x0},{cy + d} H{cx}")
                if right:
                    parts.append(f"M{cx},{cy - d} H{x1} M{cx},{cy + d} H{x1}")
                if up:
                    parts.append(f"M{cx - d},{y0} V{cy} M{cx + d},{y0} V{cy}")
                if down:
                    parts.append(f"M{cx - d},{cy} V{y1} M{cx + d},{cy} V{y1}")
            return (
                f'<path d="{" ".join(parts)}" fill="none" stroke="{fg}" '
                f'stroke-width="1.2" stroke-linecap="square"{opacity}/>'
            )

        # Single / heavy / rounded box drawing
        sw = "2.0" if (left == 2 or right == 2 or up == 2 or down == 2) else "1.35"
        if left:
            parts.append(f"M{x0},{cy} H{cx}")
        if right:
            parts.append(f"M{cx},{cy} H{x1}")
        if up:
            parts.append(f"M{cx},{y0} V{cy}")
        if down:
            parts.append(f"M{cx},{cy} V{y1}")
        return (
            f'<path d="{" ".join(parts)}" fill="none" stroke="{fg}" '
            f'stroke-width="{sw}" stroke-linecap="square"{opacity}/>'
        )

    return None


def _append_text_style_attrs(attrs: list[str], cell: Cell) -> None:
    if cell.bold:
        attrs.append('font-weight="700"')
    if cell.dim:
        attrs.append('fill-opacity="0.65"')
    if cell.italic:
        attrs.append('font-style="italic"')
    decorations: list[str] = []
    if cell.underline:
        decorations.append("underline")
    if cell.strikethrough:
        decorations.append("line-through")
    if decorations:
        attrs.append(f'text-decoration="{" ".join(decorations)}"')


def render_frame_svg(
    grid: list[list[Cell]],
    width: int,
    height: int,
    default_bg: str = DEFAULT_BG,
    cell_w: int = 9,
    cell_h: int = 18,
    pad: int = 12,
) -> str:
    svg_w = width * cell_w + pad * 2
    svg_h = height * cell_h + pad * 2
    lines: list[str] = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{svg_w}" height="{svg_h}" viewBox="0 0 {svg_w} {svg_h}">',
        f'<rect width="{svg_w}" height="{svg_h}" rx="8" ry="8" fill="{default_bg}" stroke="#30363d" stroke-width="1"/>',
        '<g font-family="\'JetBrains Mono\',\'DejaVu Sans Mono\',\'Liberation Mono\',monospace" font-size="15">',
    ]

    # 1. Background rectangles (run-length encoded per row)
    for r, row in enumerate(grid):
        y = pad + r * cell_h
        c = 0
        while c < width:
            cell = row[c]
            eff_bg = cell.fg if cell.reverse else cell.bg
            if eff_bg.lower() == default_bg.lower():
                c += 1
                continue
            start_c = c
            while c < width:
                nxt = row[c]
                nxt_bg = nxt.fg if nxt.reverse else nxt.bg
                if nxt_bg.lower() != eff_bg.lower():
                    break
                c += 1
            x = pad + start_c * cell_w
            w = (c - start_c) * cell_w
            lines.append(f'<rect x="{x}" y="{y}" width="{w}" height="{cell_h}" fill="{eff_bg}"/>')

    # 2. Foreground text & vector box-drawing / block glyphs
    for r, row in enumerate(grid):
        y_top = pad + r * cell_h
        y_base = y_top + 14
        c = 0
        while c < width:
            cell = row[c]
            if cell.cont or not cell.ch or cell.ch == " ":
                c += 1
                continue
            eff_fg = cell.bg if cell.reverse else cell.fg
            x = pad + c * cell_w
            vec = _render_vector_cell_glyph(cell.ch, x, y_top, cell_w, cell_h, eff_fg, cell.dim)
            if vec is not None:
                lines.append(vec)
                c += 1
                continue

            cp = ord(cell.ch[0])
            if cp > 0x7E:
                attrs = [f'x="{x}"', f'y="{y_base}"', f'fill="{eff_fg}"']
                _append_text_style_attrs(attrs, cell)
                lines.append(f'<text {" ".join(attrs)}>{html.escape(cell.ch)}</text>')
                c += 1
                continue

            # Group contiguous ASCII characters sharing identical style
            start_c = c
            style_sig = (eff_fg, cell.bold, cell.dim, cell.italic, cell.underline, cell.strikethrough)
            chars: list[str] = []
            while c < width:
                nxt = row[c]
                if nxt.cont or not nxt.ch or ord(nxt.ch[0]) > 0x7E:
                    break
                nxt_fg = nxt.bg if nxt.reverse else nxt.fg
                nxt_sig = (nxt_fg, nxt.bold, nxt.dim, nxt.italic, nxt.underline, nxt.strikethrough)
                if nxt_sig != style_sig:
                    break
                chars.append(nxt.ch)
                c += 1

            text_str = "".join(chars)
            if not text_str.strip() and not cell.underline and not cell.strikethrough:
                continue
            x = pad + start_c * cell_w
            attrs = [
                f'x="{x}"',
                f'y="{y_base}"',
                f'fill="{eff_fg}"',
                'xml:space="preserve"',
            ]
            _append_text_style_attrs(attrs, cell)
            lines.append(f'<text {" ".join(attrs)}>{html.escape(text_str)}</text>')

    lines.append("</g></svg>")
    return "\n".join(lines)


def parse_cast_frames(
    cast_path: Path,
    min_frame_interval: float = 0.06,
    max_frames: int = 42,
) -> tuple[dict, list[tuple[float, list[list[Cell]]]]]:
    raw_lines = [
        (line_no, line.strip())
        for line_no, line in enumerate(cast_path.read_text(encoding="utf-8").splitlines(), start=1)
        if line.strip()
    ]
    if not raw_lines:
        raise ValueError(f"Empty cast file: {cast_path}")

    header_line_no, header_raw = raw_lines[0]
    try:
        header = json.loads(header_raw)
    except json.JSONDecodeError as exc:
        raise ValueError(f"Invalid JSON header in {cast_path}:{header_line_no}: {exc}") from exc
    if not isinstance(header, dict):
        raise ValueError(f"Expected JSON object header in {cast_path}:{header_line_no}")

    width = int(header.get("width", 84))
    height = int(header.get("height", 20))
    theme = header.get("theme") or {}
    default_bg = theme.get("bg", DEFAULT_BG) if isinstance(theme, dict) else DEFAULT_BG
    default_fg = theme.get("fg", DEFAULT_FG) if isinstance(theme, dict) else DEFAULT_FG

    sim = TerminalSimulator(width=width, height=height, default_fg=default_fg, default_bg=default_bg)

    events: list[tuple[float, str]] = []
    for line_no, line in raw_lines[1:]:
        try:
            ev = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ValueError(f"Invalid JSON event in {cast_path}:{line_no}: {exc}") from exc
        if isinstance(ev, list) and len(ev) >= 3 and ev[1] == "o":
            events.append((float(ev[0]), str(ev[2])))

    if not events:
        return header, [(1.0, sim.snapshot())]

    snapshots: list[tuple[float, tuple, list[list[Cell]]]] = []
    idx = 0
    while idx < len(events):
        t0 = events[idx][0]
        sim.feed(events[idx][1])
        idx += 1
        while idx < len(events) and (events[idx][0] - t0) < min_frame_interval:
            sim.feed(events[idx][1])
            idx += 1
        t_next = events[idx][0] if idx < len(events) else (t0 + 1.2)
        duration = max(0.06, min(2.0, t_next - t0))
        key = sim.snapshot_key()
        if snapshots and snapshots[-1][1] == key:
            prev_dur, prev_key, prev_grid = snapshots[-1]
            snapshots[-1] = (min(2.5, prev_dur + duration), prev_key, prev_grid)
        else:
            snapshots.append((duration, key, sim.snapshot()))

    if len(snapshots) > max_frames:
        if max_frames <= 1:
            snapshots = [snapshots[-1]]
        else:
            step = (len(snapshots) - 1) / float(max_frames - 1)
            picked_indices = sorted({round(i * step) for i in range(max_frames)})
            snapshots = [snapshots[i] for i in picked_indices]

    if snapshots:
        last_dur, last_key, last_grid = snapshots[-1]
        snapshots[-1] = (max(last_dur, 1.2), last_key, last_grid)

    return header, [(dur, grid) for dur, _, grid in snapshots]


def _resolve_gif_tools() -> tuple[str, str]:
    rsvg = shutil.which("rsvg-convert")
    if not rsvg:
        raise RuntimeError(
            "rsvg-convert not found in PATH (install librsvg2-bin / librsvg)"
        )
    magick = shutil.which("magick") or shutil.which("convert")
    if not magick:
        raise RuntimeError(
            "ImageMagick ('magick' or 'convert') not found in PATH (install imagemagick)"
        )
    return rsvg, magick


def convert_cast_to_gif(
    cast_path: Path,
    gif_path: Path | None,
    svg_path: Path | None = None,
    svg_frame_index: int = 0,
    rsvg_bin: str | None = None,
    magick_bin: str | None = None,
) -> None:
    header, frames = parse_cast_frames(cast_path)
    width = int(header.get("width", 84))
    height = int(header.get("height", 20))
    theme = header.get("theme") or {}
    default_bg = theme.get("bg", DEFAULT_BG) if isinstance(theme, dict) else DEFAULT_BG

    if svg_path is not None and frames:
        idx = svg_frame_index if svg_frame_index >= 0 else len(frames) + svg_frame_index
        idx = max(0, min(len(frames) - 1, idx))
        svg_content = render_frame_svg(frames[idx][1], width, height, default_bg=default_bg)
        svg_path.parent.mkdir(parents=True, exist_ok=True)
        svg_path.write_text(svg_content, encoding="utf-8")

    if gif_path is None:
        return

    if rsvg_bin is None or magick_bin is None:
        rsvg_bin, magick_bin = _resolve_gif_tools()

    with tempfile.TemporaryDirectory(prefix="qmltui_gif_") as tmp_dir:
        tmp = Path(tmp_dir)
        png_paths: list[Path] = []
        for i, (_, grid) in enumerate(frames):
            svg_file = tmp / f"frame_{i:04d}.svg"
            png_file = tmp / f"frame_{i:04d}.png"
            svg_file.write_text(render_frame_svg(grid, width, height, default_bg=default_bg), encoding="utf-8")
            subprocess.run(
                [rsvg_bin, str(svg_file), "-o", str(png_file)],
                check=True,
            )
            png_paths.append(png_file)

        cmd: list[str] = [magick_bin, "-loop", "0"]
        for (dur, _), png_file in zip(frames, png_paths):
            ticks = max(4, int(round(dur * 100)))
            cmd.extend(["-delay", str(ticks), str(png_file)])
        cmd.extend(["-layers", "Optimize", str(gif_path)])
        gif_path.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(cmd, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description="Render asciicast v2 (.cast) files to animated GIFs and static SVGs.")
    parser.add_argument("inputs", nargs="+", help="Input .cast file(s)")
    parser.add_argument("-o", "--output", help="Output .gif path (only valid with a single input)")
    parser.add_argument("--svg", help="Optional static .svg output path (only valid with a single input)")
    parser.add_argument("--svg-only", action="store_true", help="Only write --svg without rendering a GIF")
    parser.add_argument("--svg-frame", type=int, default=0, help="Frame index to export for --svg (default: 0)")
    parser.add_argument("-j", "--jobs", type=int, default=4, help="Max worker threads (capped at 8)")
    args = parser.parse_args()

    input_paths = [Path(p) for p in args.inputs]
    for p in input_paths:
        if not p.is_file():
            print(f"error: input .cast file not found: {p}", file=sys.stderr)
            return 1

    if len(input_paths) > 1 and (args.output or args.svg or args.svg_only):
        print("error: -o/--output, --svg, and --svg-only require a single input .cast file", file=sys.stderr)
        return 1

    if args.svg_only and not args.svg:
        print("error: --svg-only requires --svg <output.svg>", file=sys.stderr)
        return 1

    try:
        rsvg_bin: str | None = None
        magick_bin: str | None = None
        if not args.svg_only:
            rsvg_bin, magick_bin = _resolve_gif_tools()

        if len(input_paths) == 1:
            out_gif = None if args.svg_only else (Path(args.output) if args.output else input_paths[0].with_suffix(".gif"))
            svg_out = Path(args.svg) if args.svg else None
            convert_cast_to_gif(
                input_paths[0],
                out_gif,
                svg_path=svg_out,
                svg_frame_index=args.svg_frame,
                rsvg_bin=rsvg_bin,
                magick_bin=magick_bin,
            )
            if out_gif is not None and not args.output:
                print(f"    {input_paths[0].name} -> {out_gif.name}")
            return 0

        jobs = max(1, min(8, args.jobs))

        def _worker(cast_file: Path) -> str:
            out_gif = cast_file.with_suffix(".gif")
            convert_cast_to_gif(cast_file, out_gif, rsvg_bin=rsvg_bin, magick_bin=magick_bin)
            return cast_file.name

        with ThreadPoolExecutor(max_workers=jobs) as pool:
            for name in pool.map(_worker, input_paths):
                print(f"    {name} -> {Path(name).with_suffix('.gif')}")
    except (ValueError, RuntimeError, subprocess.CalledProcessError, OSError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
