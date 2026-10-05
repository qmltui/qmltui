#!/usr/bin/env python3
"""Measure Qtui starter key-to-frame latency locally and through one SSH PTY."""

from __future__ import annotations

import argparse
import json
import math
import os
import pty
import re
import select
import shlex
import subprocess
import sys
import time
import tty
from pathlib import Path
from typing import Any


ANSI_RE = re.compile(rb"\x1b(?:\[[0-?]*[ -/]*[@-~]|\][^\x07]*(?:\x07|\x1b\\)|[()][0-2A-Z])")


def percentile(values: list[float], fraction: float) -> float:
    ordered = sorted(values)
    return ordered[max(0, math.ceil(len(ordered) * fraction) - 1)]


def summary(values: list[float]) -> dict[str, float]:
    return {
        "p50": round(percentile(values, 0.50), 3),
        "p95": round(percentile(values, 0.95), 3),
        "max": round(max(values), 3),
    }


def ssh_base(args: argparse.Namespace) -> list[str]:
    command = [
        "ssh",
        "-o", "BatchMode=yes",
        "-o", f"ConnectTimeout={args.connect_timeout}",
    ]
    for option in args.ssh_option:
        command.extend(["-o", option])
    command.append(args.target)
    return command


def run_remote(args: argparse.Namespace, command: str, timeout: float = 20.0) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ssh_base(args) + [command],
        text=True,
        capture_output=True,
        timeout=timeout,
        check=False,
    )


def measure_local(args: argparse.Namespace) -> dict[str, Any]:
    script = r'''
const samples = []
for (let index = 0; index < 30; ++index) {
    const started = Date.now()
    Test.press(index % 2 === 0 ? "F2" : "F1")
    samples.push(Date.now() - started)
}
samples.sort((left, right) => left - right)
const p95 = samples[Math.ceil(samples.length * 0.95) - 1]
Test.captureText("metrics/local-p95-" + p95 + ".txt")
if (p95 > 20)
    Test.fail("Local key-to-frame p95 was " + p95 + " ms, expected at most 20 ms")
'''.strip()
    remote = (
        "artifact_dir=$(mktemp -d /tmp/qtui-latency.XXXXXX) || exit 1; "
        f"QTUI_EVAL={shlex.quote(script)} "
        "QTUI_TEST_ARTIFACT_DIR=\"$artifact_dir\" "
        "QTUI_TEST_COLUMNS=120 QTUI_TEST_ROWS=36 QTUI_STARTER_BACKEND=observe "
        f"{shlex.quote(args.app)} >/dev/null 2>&1; app_status=$?; "
        "metric=$(find \"$artifact_dir/metrics\" -type f -name 'local-p95-*.txt' 2>/dev/null | head -n 1); "
        "printf 'QTUI_LOCAL_P95=%s\\n' \"$(basename \"$metric\" .txt | sed 's/local-p95-//')\"; "
        "printf 'QTUI_LOCAL_ARTIFACT=%s\\n' \"$artifact_dir\"; "
        "cat \"$artifact_dir/result.json\" 2>/dev/null; exit $app_status"
    )
    completed = run_remote(args, remote, timeout=30.0)
    match = re.search(r"QTUI_LOCAL_P95=(\d+)", completed.stdout)
    if not match:
        raise RuntimeError(
            "on-device benchmark did not produce a p95 value: "
            + (completed.stderr.strip() or completed.stdout.strip() or f"exit {completed.returncode}")
        )
    p95 = float(match.group(1))
    artifact = re.search(r"QTUI_LOCAL_ARTIFACT=([^\n]+)", completed.stdout)
    return {
        "p95_ms": p95,
        "limit_ms": 20.0,
        "passed": completed.returncode == 0 and p95 <= 20.0,
        "remote_artifact_dir": artifact.group(1) if artifact else None,
    }


def plain(data: bytes) -> str:
    return ANSI_RE.sub(b"", data).decode("utf-8", errors="replace")


def read_until(master: int, marker: str, timeout: float, started: float) -> tuple[bytes, float, float]:
    data = bytearray()
    first_byte_at: float | None = None
    deadline = started + timeout
    marker_seen_at: float | None = None
    quiet_deadline: float | None = None
    while time.monotonic() < deadline:
        now = time.monotonic()
        wait = min(0.05, deadline - now)
        if quiet_deadline is not None:
            wait = min(wait, max(0.0, quiet_deadline - now))
        ready, _, _ = select.select([master], [], [], wait)
        if ready:
            chunk = os.read(master, 65536)
            if not chunk:
                break
            if first_byte_at is None:
                first_byte_at = time.monotonic()
            data.extend(chunk)
            if marker in plain(data):
                marker_seen_at = time.monotonic()
                quiet_deadline = marker_seen_at + 0.030
        elif quiet_deadline is not None and time.monotonic() >= quiet_deadline:
            break
    if first_byte_at is None:
        raise TimeoutError(f"no terminal output after input; expected {marker!r}")
    if marker_seen_at is None:
        raise TimeoutError(f"frame marker {marker!r} was not rendered")
    settled_at = max(marker_seen_at, quiet_deadline or marker_seen_at)
    return bytes(data), first_byte_at, settled_at


def drain(master: int) -> None:
    while True:
        ready, _, _ = select.select([master], [], [], 0.02)
        if not ready:
            return
        try:
            if not os.read(master, 65536):
                return
        except OSError:
            return


def measure_ssh_pty(args: argparse.Namespace) -> dict[str, Any]:
    master, slave = pty.openpty()
    tty.setraw(slave)
    environment = os.environ.copy()
    environment["TERM"] = "xterm-256color"
    command = ssh_base(args)[:-1] + ["-tt", args.target,
        f"TERM=xterm-256color QTUI_STARTER_BACKEND=observe {shlex.quote(args.app)}"]
    started = time.monotonic()
    process = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=slave, env=environment, close_fds=True)
    os.close(slave)
    try:
        initial, _, initial_settled = read_until(master, "[F1] Network", args.startup_timeout, started)
        startup_ms = (initial_settled - started) * 1000.0
        first_byte_values: list[float] = []
        settle_values: list[float] = []
        byte_values: list[int] = []
        for index in range(args.samples):
            drain(master)
            if index % 2 == 0:
                key, marker = b"2", "Terminal UI device setup"
            else:
                key, marker = b"1", "[F1] Network"
            sample_started = time.monotonic()
            os.write(master, key)
            frame, first_byte_at, settled_at = read_until(
                master, marker, args.sample_timeout, sample_started
            )
            first_byte_values.append((first_byte_at - sample_started) * 1000.0)
            settle_values.append((settled_at - sample_started) * 1000.0)
            byte_values.append(len(frame))
        return {
            "samples": args.samples,
            "startup_ms": round(startup_ms, 3),
            "first_byte_ms": summary(first_byte_values),
            "settle_ms": summary(settle_values),
            "render_diff_bytes": summary([float(value) for value in byte_values]),
            "target_p95_ms": 75.0,
            "warning": percentile(settle_values, 0.95) > 75.0,
        }
    finally:
        try:
            os.write(master, b"q")
        except OSError:
            pass
        try:
            process.wait(timeout=2.0)
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                process.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                process.kill()
        os.close(master)


def ping_stats(args: argparse.Namespace) -> dict[str, Any]:
    host = args.target.rsplit("@", 1)[-1]
    completed = subprocess.run(
        ["ping", "-c", str(args.ping_count), "-i", str(args.ping_interval), host],
        text=True, capture_output=True, timeout=args.ping_count * args.ping_interval + 10, check=False,
    )
    output = completed.stdout + completed.stderr
    loss = re.search(r"([\d.]+)% packet loss", output)
    rtt = re.search(r"(?:rtt|round-trip) min/avg/max/(?:mdev|stddev) = ([\d.]+)/([\d.]+)/([\d.]+)/([\d.]+)", output)
    return {
        "count": args.ping_count,
        "interval_seconds": args.ping_interval,
        "loss_percent": float(loss.group(1)) if loss else None,
        "rtt_ms": ({"min": float(rtt.group(1)), "avg": float(rtt.group(2)),
                    "max": float(rtt.group(3)), "mdev": float(rtt.group(4))} if rtt else None),
        "exit_code": completed.returncode,
    }


def metadata(args: argparse.Namespace) -> dict[str, str]:
    remote = r'''printf 'kernel='; uname -a
printf 'image='; . /etc/os-release 2>/dev/null; printf '%s %s\n' "${NAME:-unknown}" "${VERSION:-unknown}"
iface=$(iw dev 2>/dev/null | awk '/Interface/ {print $2; exit}')
printf 'interface=%s\n' "$iface"
printf 'link='; iw dev "$iface" link 2>/dev/null | tr '\n' ';'; printf '\n'
printf 'power_save='; iw dev "$iface" get power_save 2>/dev/null | tr '\n' ' '; printf '\n'
printf 'app='; stat -c '%s bytes, mtime %y' /usr/bin/qtui_starter_template_benchmark 2>/dev/null
printf 'app_sha256='; sha256sum /usr/bin/qtui_starter_template_benchmark 2>/dev/null | awk '{print $1}'
'''
    completed = run_remote(args, remote)
    if completed.returncode != 0 and not completed.stdout:
        return {"error": completed.stderr.strip() or f"ssh exit {completed.returncode}"}
    values: dict[str, str] = {}
    for line in completed.stdout.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value.strip()
    return values


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target", help="SSH destination, for example root@192.168.1.35")
    parser.add_argument("--app", default="/usr/bin/qtui_starter_template_benchmark")
    parser.add_argument("--samples", type=int, default=30)
    parser.add_argument("--ping-count", type=int, default=20)
    parser.add_argument("--ping-interval", type=float, default=0.2)
    parser.add_argument("--connect-timeout", type=int, default=5)
    parser.add_argument("--startup-timeout", type=float, default=10.0)
    parser.add_argument("--sample-timeout", type=float, default=2.0)
    parser.add_argument("--ssh-option", action="append", default=[])
    parser.add_argument("--output", type=Path, help="also write the JSON report to this path")
    args = parser.parse_args()
    if args.samples < 30:
        parser.error("--samples must be at least 30")
    return args


def main() -> int:
    args = parse_args()
    report: dict[str, Any] = {
        "schema_version": 1,
        "target": args.target,
        "measured_at_unix": int(time.time()),
    }
    try:
        report["metadata"] = metadata(args)
        report["ping"] = ping_stats(args)
        report["local"] = measure_local(args)
        report["ssh"] = measure_ssh_pty(args)
        warnings: list[str] = []
        if report["ssh"]["warning"]:
            warnings.append("SSH settle p95 exceeds the 75 ms interactive target")
        report["warnings"] = warnings
        report["status"] = "passed" if report["local"]["passed"] else "failed"
    except Exception as error:
        report["status"] = "error"
        report["error"] = str(error)
    encoded = json.dumps(report, indent=2, sort_keys=True)
    print(encoded)
    if args.output:
        args.output.write_text(encoded + "\n", encoding="utf-8")
    return 0 if report["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
