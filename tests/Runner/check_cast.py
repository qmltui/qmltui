#!/usr/bin/env python3
"""Validate an asciicast v2 file written by the Runner.

Checks the header, that event times never decrease, that every event has a
known code, that no terminal query (DA, kitty keyboard, sync-output probe) was
recorded, and that each --marker label and --text substring is present.
"""
import argparse
import json
import sys


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("cast")
    parser.add_argument("--marker", action="append", default=[])
    parser.add_argument("--text", action="append", default=[])
    parser.add_argument("--min-events", type=int, default=1)
    parser.add_argument("--resize")
    parser.add_argument("--min-duration", type=float, default=0)
    args = parser.parse_args()

    with open(args.cast, encoding="utf-8") as handle:
        lines = [line for line in handle.read().split("\n") if line]
    if not lines:
        print("empty cast"); return 1
    header = json.loads(lines[0])
    for key in ("version", "width", "height"):
        if key not in header:
            print(f"header lacks {key}: {header}"); return 1
    if header["version"] != 2:
        print(f"unexpected version {header['version']}"); return 1

    last = -1.0
    output = ""
    markers = []
    outputs = 0
    resized = False
    for raw in lines[1:]:
        event = json.loads(raw)
        if not (isinstance(event, list) and len(event) == 3):
            print(f"malformed event {raw}"); return 1
        time, code, data = event
        if not isinstance(time, (int, float)) or time < last:
            print(f"time went backwards at {raw}"); return 1
        last = float(time)
        if code == "o":
            outputs += 1
            output += data
        elif code == "m":
            markers.append(data)
        elif code == "r":
            cols, rows = data.split("x")
            int(cols); int(rows)
            if data == args.resize:
                resized = True
        else:
            print(f"unknown event code {code}"); return 1

    if args.resize and not resized:
        print(f"missing resize {args.resize}"); return 1
    if last < args.min_duration:
        print(f"duration {last} below {args.min_duration}"); return 1
    if "\x1b[38;2;" not in output and "\x1b[48;2;" not in output:
        print("missing TrueColor output"); return 1
    for probe in ("\x1b[c", "\x1b[?u", "\x1b[?2026$p", "\x1b[16t"):
        if probe in output:
            print(f"terminal probe {probe!r} recorded"); return 1
    if outputs < args.min_events:
        print(f"only {outputs} output events"); return 1
    for label in args.marker:
        if label not in markers:
            print(f"marker {label!r} missing; found {markers}"); return 1
    for text in args.text:
        if text not in output:
            print(f"text {text!r} not in output"); return 1
    print(f"ok: {outputs} output events, {len(markers)} markers, {header['width']}x{header['height']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
