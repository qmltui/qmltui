# CI Runners: GitHub-Hosted Default and Optional Self-Hosted Mode

By default, `.github/workflows/ci.yml` runs every job on GitHub-hosted runners (`ubuntu-24.04`, `windows-2022`, and `macos-latest`), which are free on public GitHub repositories and install cached Qt `6.8.*` toolchains with `jurplel/install-qt-action@v4`.

Maintainers who work on a private mirror (where GitHub bills Actions minutes) or want faster turnaround against local pre-installed toolchains can opt into self-hosted Linux and Windows runners by setting the repository Actions variable `QMLTUI_SELF_HOSTED=1` under **Settings > Secrets and variables > Actions > Variables**.

## Trust Boundary for External Pull Requests

Never execute untrusted pull-request code on a persistent self-hosted machine. Even when `QMLTUI_SELF_HOSTED=1` is enabled, `ci.yml` gates `runs-on` with:

```yaml
(vars.QMLTUI_SELF_HOSTED == '1' && (github.event_name != 'pull_request' || github.event.pull_request.head.repo.full_name == github.repository))
```

As a result:

- **External fork pull requests** (`head.repo.full_name != github.repository`) always run on ephemeral GitHub-hosted runners (`ubuntu-24.04`, `windows-2022`, `macos-latest`). They never access self-hosted hardware and never block in "Queued" if a developer workstation is offline.
- **Trusted runs** (`push`, `workflow_dispatch`, and same-repository pull requests) use `[self-hosted, linux]` and `[self-hosted, windows]` when `QMLTUI_SELF_HOSTED=1`, and fall back to GitHub-hosted runners whenever `QMLTUI_SELF_HOSTED` is unset or set to `0`.

## Registering Optional Self-Hosted Runners

Each runner is a background agent that polls GitHub for jobs. Fetch a one-hour registration token from **Settings > Actions > Runners > New self-hosted runner** right before running `config.sh` or `config.cmd`.

### Linux Runner (Native or WSL)

When running on `[self-hosted, linux]`, `ci.yml` reads the desktop Qt prefix from `QMLTUI_QT_LINUX`, and the `wasm` compile-guard job reads the single-threaded Qt for WebAssembly prefix from `QMLTUI_QT_WASM` plus Emscripten from `EMSDK` (defaulting to `~/emsdk`). CMake, Ninja, `python3`, `clang-format` (`23.1.3` to match GitHub-hosted CI), and `clang++` / `llvm-cov` / `llvm-profdata` (for the `coverage` job) come from the host distribution.

```bash
mkdir -p ~/actions-runner && cd ~/actions-runner
curl -o actions-runner-linux-x64.tar.gz -L \
  https://github.com/actions/runner/releases/latest/download/actions-runner-linux-x64-$(curl -s https://api.github.com/repos/actions/runner/releases/latest | grep -oP '"tag_name": "v\K[^"]+').tar.gz
tar xzf actions-runner-linux-x64.tar.gz
./config.sh --url https://github.com/qmltui/qmltui --token <TOKEN> \
  --name wsl-linux --labels linux --unattended
echo "QMLTUI_QT_LINUX=$HOME/Qt/6.8.6/gcc_64" >> .env
echo "QMLTUI_QT_WASM=$HOME/Qt/6.8.6/wasm_singlethread" >> .env
echo "EMSDK=$HOME/emsdk" >> .env
```

Keep the runner active:

- **Without `systemd` (default WSL)**: start it in a detached `tmux` session:

  ```bash
  tmux new -d -s runner 'cd ~/actions-runner && ./run.sh'
  ```

- **With `systemd`** (native Linux, or WSL with `[boot] systemd=true` in `/etc/wsl.conf`): install it as a service so it restarts automatically on boot:

  ```bash
  sudo ./svc.sh install && sudo ./svc.sh start
  ```

### Windows Runner

When running on `[self-hosted, windows]`, `ci.yml` reads the MSVC Qt prefix from `QMLTUI_QT_WINDOWS` and uses Visual Studio 2022 with the Desktop C++ workload (`ilammy/msvc-dev-cmd`). Put CMake and Ninja on the runner `PATH` via `.path`. In an elevated PowerShell:

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

The Windows service reads `.env` and `.path` at startup; restart `actions.runner.*` after editing either file.

## Verifying or Deregistering Runners

Trigger `CI` from the **Actions** tab or push a branch. When `QMLTUI_SELF_HOSTED=1` is set, trusted `linux` and `windows` jobs display the self-hosted runner names above; if a job stays in "Queued", either the runner is offline, its labels are missing `linux` or `windows`, or you should clear `QMLTUI_SELF_HOSTED` to run on GitHub-hosted images.

To remove a runner, generate a removal token on **Settings > Actions > Runners** and run `./config.sh remove --token <TOKEN>` (Linux) or `.\config.cmd remove --token <TOKEN>` (Windows).

## Job Matrix and Runner Routing

| Job | Default Runner (and all fork PRs) | Opt-in Runner (`QMLTUI_SELF_HOSTED=1`, non-fork) | Trigger |
| :--- | :--- | :--- | :--- |
| `linux` (`Private Qt` `ON` and `OFF`, format/docs/skills checks) | `ubuntu-24.04` | `[self-hosted, linux]` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `sanitizers` (`ASan` + `UBSan`) | `ubuntu-24.04` | `[self-hosted, linux]` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `coverage` (Clang `llvm-cov` report artifact) | `ubuntu-24.04` | `[self-hosted, linux]` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `coverage-badge` (publishes `coverage.svg` & `summary.md` to `badges` branch) | `ubuntu-24.04` | `ubuntu-24.04` | `main` pushes (requires `coverage`, `contents: write`) |
| `windows` (`Private Qt` `ON`, MSVC 2022) | `windows-2022` | `[self-hosted, windows]` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `macos` (`Private Qt` `ON`) | `macos-latest` | `macos-latest` | Every trigger on public repos; `workflow_dispatch` and `v*` tags on private repos |
| `quickstart` (`examples/helloworld` via `FetchContent`) | `ubuntu-24.04` | `[self-hosted, linux]` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `wasm` (`examples/helloworld` Wasm compile-guard) | `ubuntu-24.04` | `[self-hosted, linux]` | Wasm/Core/Controls/CMake changes, `workflow_dispatch`, and `v*` tags (full Wasm playground & Pages site build runs in `qmltui/qmltui.github.io`) |
| `static-packaging` (`BUILD_SHARED_LIBS=OFF`, system `utf8proc` + `libvterm`, installed consumer) | `ubuntu-24.04` | `ubuntu-24.04` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
| `reuse` (`reuse lint`) | `ubuntu-24.04` | `ubuntu-24.04` | `main` pushes, `v*` tags, pull requests, `workflow_dispatch` |
