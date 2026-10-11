#!/usr/bin/env python3
"""Check that tutorial fences match their marked repository source files."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PATTERN = re.compile(r'<!-- include: ([^\n]+) -->\n```[^\n]+\n(.*?)^```', re.M | re.S)
failures = []
count = 0
for page in sorted((ROOT / 'docs/tutorials').glob('*.md')):
    text = page.read_text()
    matches = list(PATTERN.finditer(text))
    if text.count('<!-- include:') != len(matches):
        failures.append(f'{page}: malformed include marker')
    for match in matches:
        count += 1
        source = ROOT / match[1]
        if not source.is_file() or source.read_text() != match[2]:
            failures.append(f'{page.name}: fence differs from {match[1]}')
for failure in failures:
    print(failure)
print(f'{count} source includes checked; {len(failures)} failures')
sys.exit(bool(failures) or count == 0)
