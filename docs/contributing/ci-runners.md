# CI on Your Own Machines: Self-Hosted Runners

GitHub bills Actions minutes on a private repository: 2,000 a month on the
Free plan, with Windows counting twice and macOS ten times. The full matrix
cost about 280 billed minutes per push, so a busy day empties the month. Jobs
that run on a self-hosted runner are not billed on any plan, and the developer
machine already has the toolchains, so `ci.yml` runs Linux and Windows there
and keeps hosted macOS for `workflow_dispatch` and tags only. When the
repository goes public, hosted runners become free and the `runs-on` lines can
return to GitHub's images.

Each runner is a small agent that polls GitHub for jobs. It needs the
registration token from **Settings, Actions, Runners, New self-hosted runner**
in the repository; the token is valid for an hour, so fetch it right before
each `config` step below. Jobs queue while a runner is offline and start when
it comes back.

## Linux runner (WSL)

The workflows expect the desktop Qt prefix in `QMLTUI_QT_LINUX`, and the `wasm`
compile-guard job in `ci.yml` expects the Qt for WebAssembly single-threaded
prefix in `QMLTUI_QT_WASM` plus Emscripten (`EMSDK`, defaulting to `~/emsdk`).
CMake, Ninja, and `clang++` / `llvm-cov` / `llvm-profdata` (for the `coverage`
job) come from the distribution.

```bash
mkdir -p ~/actions-runner && cd ~/actions-runner
curl -o actions-runner-linux-x64.tar.gz -L \
  https://github.com/actions/runner/releases/latest/download/actions-runner-linux-x64-$(curl -s https://api.github.com/repos/actions/runner/releases/latest | grep -oP '"tag_name": "v\K[^"]+').tar.gz
tar xzf actions-runner-linux-x64.tar.gz
./config.sh --url https://github.com/qmltui/qmltui --token <TOKEN> \
  --name wsl-linux --labels linux --unattended
echo 'QMLTUI_QT_LINUX=/home/yosri/Qt/6.8.6/gcc_64' >> .env
echo 'QMLTUI_QT_WASM=/home/yosri/Qt/6.8.6/wasm_singlethread' >> .env
echo 'EMSDK=/home/yosri/emsdk' >> .env
```

Then keep it running. Without systemd in WSL (the default here), start it in a
detached session and leave that WSL instance open:

```bash
tmux new -d -s runner 'cd ~/actions-runner && ./run.sh'
```

With systemd enabled (`[boot] systemd=true` in `/etc/wsl.conf`, then
`wsl --shutdown` once from Windows), install it as a service instead so it
survives reboots of the WSL instance:

```bash
sudo ./svc.sh install && sudo ./svc.sh start
```

## Windows runner

The workflow expects the MSVC Qt prefix in `QMLTUI_QT_WINDOWS` and Visual
Studio 2022 with the C++ workload (`ilammy/msvc-dev-cmd` finds it). CMake and
Ninja from the Qt installer are put on the runner's PATH through `.path`.
In an elevated PowerShell:

```powershell
mkdir C:\actions-runner; cd C:\actions-runner
$v = (Invoke-RestMethod https://api.github.com/repos/actions/runner/releases/latest).tag_name.TrimStart('v')
Invoke-WebRequest -Uri "https://github.com/actions/runner/releases/download/v$v/actions-runner-win-x64-$v.zip" -OutFile runner.zip
Expand-Archive runner.zip -DestinationPath .
.\config.cmd --url https://github.com/qmltui/qmltui --token <TOKEN> --name win-host --labels windows --runasservice --unattended
Add-Content .env 'QMLTUI_QT_WINDOWS=C:\Qt\6.8.3\msvc2022_64'
Add-Content .path "C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;$env:PATH"
Restart-Service "actions.runner.*"
```

`--runasservice` registers a Windows service, so the runner is up whenever
the machine is. The `.env` and `.path` files are read at service start; edit
them and restart the service to change tools.

## Checking

Push a commit or run the workflow from the Actions tab. The `linux` and
`windows` jobs should show the runner names above. If a job sits in "Queued",
the matching runner is offline or its labels do not include `linux` or
`windows`. To remove a runner: `./config.sh remove --token <TOKEN>` (Linux) or
`.\config.cmd remove --token <TOKEN>` (Windows) with a removal token from the
same settings page.

## What runs where

| Job | Runner | Trigger |
| :--- | :--- | :--- |
| `linux` (Behavior on and off) | self-hosted `linux` | every push and pull request |
| `coverage` (Clang `llvm-cov`) | self-hosted `linux` | every push and pull request |
| `windows` (Behavior on) | self-hosted `windows` | every push and pull request |
| `quickstart` (`examples/helloworld`) | self-hosted `linux` | every push and pull request |
| `wasm` (`examples/helloworld` Wasm compile-guard) | self-hosted `linux` | Wasm/Core/Controls/CMake pushes, pull requests, `workflow_dispatch`, and `v*` tags (full Wasm playground & Pages site build runs in `qmltui/website`) |
| `macos` | hosted `macos-14` | `workflow_dispatch` and `v*` tags |
| `reuse` | hosted `ubuntu-24.04` | pull requests and `workflow_dispatch` |
