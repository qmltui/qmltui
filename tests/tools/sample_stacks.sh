#!/bin/bash
# Usage: tests/tools/sample_stacks.sh <build>/tests/Core/benchmark_view_phases <View> <delegate> [samples]
# Needs gdb and a RelWithDebInfo build; the benchmark opts in to ptrace with PR_SET_PTRACER.
# Set QML2_IMPORT_PATH=<build>/qml when running from a tree without generated qt.conf.
# Poor man's sampling profiler: $1 = benchmark binary, $2 = view, $3 = delegate, $4 = samples
BIN=$1; VIEW=$2; DEL=$3; N=${4:-40}
OUT=$(dirname "$0")/samples_${VIEW}_${DEL}.txt; : > "$OUT"
"$BIN" --loop "$VIEW" "$DEL" 60 >/dev/null 2>&1 &
PID=$!
sleep 1.5
for i in $(seq 1 "$N"); do
  echo "=== sample $i" >> "$OUT"
  gdb -p "$PID" -batch -ex "bt 30" 2>/dev/null | grep '^#' >> "$OUT"
  sleep 0.15
done
kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null
python3 - "$OUT" <<'PY'
import sys,re,collections
text=open(sys.argv[1]).read()
samples=[s for s in text.split('=== sample')[1:] if '#0' in s]
incl=collections.Counter(); top=collections.Counter()
for s in samples:
    funcs=[]
    for line in s.splitlines():
        m=re.match(r'#\d+\s+(?:0x[0-9a-f]+ in )?(.+?) \(', line)
        if m:
            f=m.group(1).strip()
            f=re.sub(r'<.*?>','<>',f)
            funcs.append(f)
    if not funcs: continue
    top[funcs[0]]+=1
    for f in set(funcs): incl[f]+=1
n=len(samples)
print(f"{n} samples")
print("-- inclusive (share of samples with the function on the stack)")
for f,c in incl.most_common(28): print(f"{100*c/n:5.0f}%  {f[:110]}")
print("-- leaf (top of stack)")
for f,c in top.most_common(8): print(f"{100*c/n:5.0f}%  {f[:110]}")
PY
