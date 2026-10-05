#!/usr/bin/env bash
# ==============================================================================
# Qtui Embedded Fast Dev Loop Tool
# Cross-compiles, deploys over SSH, and runs Qtui apps on /dev/tty1 in < 2 seconds.
# Supports both in-tree examples and arbitrary external CMake applications.
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

TARGET_IP="${QTUI_TARGET_IP:-192.168.1.35}"
TARGET_USER="${QTUI_TARGET_USER:-root}"
ACTION="${1:-dev}"
APP_INPUT="${2:-starter_template}"

# Handle argument permutations: e.g. ./qtui-dev.sh dev 192.168.1.35
if [[ "${APP_INPUT}" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    TARGET_IP="${APP_INPUT}"
    APP_INPUT="starter_template"
fi

if [ -n "$3" ] && [[ "$3" =~ ^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    TARGET_IP="$3"
fi

# Detect application type (External directory, Pre-compiled binary, or In-tree example)
IS_EXTERNAL_DIR=0
IS_DIRECT_BINARY=0

if [ -f "${APP_INPUT}" ]; then
    # Direct binary specified: e.g. /path/to/binary
    IS_DIRECT_BINARY=1
    LOCAL_BIN="$(cd "$(dirname "${APP_INPUT}")" && pwd)/$(basename "${APP_INPUT}")"
    BIN_NAME="$(basename "${LOCAL_BIN}")"
    REMOTE_BIN="/usr/bin/${BIN_NAME}"
    APP_NAME="${BIN_NAME}"
elif [ -d "${APP_INPUT}" ] && [ -f "${APP_INPUT}/CMakeLists.txt" ]; then
    # External CMake directory specified: e.g. /path/to/external_project
    IS_EXTERNAL_DIR=1
    APP_DIR="$(cd "${APP_INPUT}" && pwd)"
    APP_NAME="$(basename "${APP_DIR}")"
    BUILD_DIR="${APP_DIR}/build-arm64"
    BIN_NAME="${APP_NAME}"
    LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
    REMOTE_BIN="/usr/bin/${BIN_NAME}"
else
    # In-tree example application name
    APP_NAME="${APP_INPUT}"
    case "${APP_NAME}" in
        starter_template|starter)
            APP_DIR="${REPO_ROOT}/examples/apps/starter_template"
            WORK_DIR="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0"
            BUILD_DIR="${WORK_DIR}/build"
            SRC_DIR="${WORK_DIR}/git"
            RECIPE_NAME="qtui-starter-template"
            BIN_NAME="qtui_starter_template"
            LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
            REMOTE_BIN="/usr/bin/qtui_starter_template"
            ;;
        snake_game|snake)
            APP_DIR="${REPO_ROOT}/examples/apps/snake_game"
            WORK_DIR="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0"
            BUILD_DIR="${WORK_DIR}/build"
            SRC_DIR="${WORK_DIR}/git"
            RECIPE_NAME="qtui"
            BIN_NAME="examples/apps/snake_game/qtui_snake_game"
            LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
            REMOTE_BIN="/usr/bin/qtui_snake_game"
            ;;
        system_dashboard|dashboard)
            APP_DIR="${REPO_ROOT}/examples/apps/system_dashboard"
            WORK_DIR="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0"
            BUILD_DIR="${WORK_DIR}/build"
            SRC_DIR="${WORK_DIR}/git"
            RECIPE_NAME="qtui"
            BIN_NAME="examples/apps/system_dashboard/qtui_system_dashboard"
            LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
            REMOTE_BIN="/usr/bin/qtui_system_dashboard"
            ;;
        device_cockpit|cockpit)
            APP_DIR="${REPO_ROOT}/examples/apps/device_cockpit"
            WORK_DIR="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0"
            BUILD_DIR="${WORK_DIR}/build"
            SRC_DIR="${WORK_DIR}/git"
            RECIPE_NAME="qtui"
            BIN_NAME="examples/apps/device_cockpit/qtui_device_cockpit"
            LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
            REMOTE_BIN="/usr/bin/qtui_device_cockpit"
            ;;
        *)
            APP_DIR="${REPO_ROOT}/examples/apps/${APP_NAME}"
            WORK_DIR="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0"
            BUILD_DIR="${WORK_DIR}/build"
            SRC_DIR="${WORK_DIR}/git"
            RECIPE_NAME="qtui"
            BIN_NAME="examples/apps/${APP_NAME}/qtui_${APP_NAME}"
            LOCAL_BIN="${BUILD_DIR}/${BIN_NAME}"
            REMOTE_BIN="/usr/bin/qtui_${APP_NAME}"
            ;;
    esac
fi

show_help() {
    cat << 'HELPEOF'
Qtui Fast Embedded Dev-Loop Tool (No Reflashing Required)

Usage: ./meta-qtui/scripts/qtui-dev.sh [action] [target_app] [target_ip]

Actions:
  dev             (Default) Build -> Deploy -> Run live on /dev/tty1 in ~2s
  build           Cross-compile for ARM64 (aarch64 Cortex-A55)
  deploy          SCP compiled binary to target board over SSH
  run-tty1        Launch the deployed application on physical screen (/dev/tty1)
  run-ssh         Launch the deployed application interactively inside SSH terminal
  restart-tty1    Restart the default qtui-app system service on /dev/tty1
  stop-tty1       Stop the UI services on /dev/tty1 (frees display)
  status          Show board connection, active service, and screen state

Target Apps:
  1. In-tree example name:   starter_template, snake_game, system_dashboard, device_cockpit, etc.
  2. External project path:  /path/to/my_external_qtui_app
  3. Pre-built binary path:  /path/to/my_app/build/my_app

Examples:
  ./meta-qtui/scripts/qtui-dev.sh dev starter_template 192.168.1.35
  ./meta-qtui/scripts/qtui-dev.sh dev /path/to/external_app 192.168.1.35
  ./meta-qtui/scripts/qtui-dev.sh run-ssh snake_game 192.168.1.35
  ./meta-qtui/scripts/qtui-dev.sh restart-tty1 192.168.1.35
HELPEOF
}

cmd_build() {
    echo "🔨 [1/3] Cross-compiling '${APP_NAME}' for aarch64..."
    local start_time=$(date +%s%N)

    if [ "${IS_DIRECT_BINARY}" = "1" ]; then
        echo "ℹ️ Using pre-built binary: ${LOCAL_BIN}"
        return 0
    fi

    if [ "${IS_EXTERNAL_DIR}" = "1" ]; then
        "${SCRIPT_DIR}/qtui-cross-cmake.sh" -S "${APP_DIR}" -B "${BUILD_DIR}"
        ninja -C "${BUILD_DIR}"

        # If target binary name differed from directory name, locate the main executable
        if [ ! -f "${LOCAL_BIN}" ]; then
            LOCAL_BIN="$(find "${BUILD_DIR}" -maxdepth 1 -type f -executable | head -n 1)"
            BIN_NAME="$(basename "${LOCAL_BIN}")"
            REMOTE_BIN="/usr/bin/${BIN_NAME}"
        fi
    else
        # In-tree repository app build
        if [ "${RECIPE_NAME}" = "qtui-starter-template" ]; then
            if [ -d "${APP_DIR}" ] && [ -d "${SRC_DIR}" ]; then
                rsync -a --exclude='.git' --exclude='build' "${APP_DIR}/" "${SRC_DIR}/"
            fi
        else
            if [ -d "${REPO_ROOT}/examples" ] && [ -d "${SRC_DIR}/examples" ]; then
                rsync -a --exclude='.git' --exclude='build' "${REPO_ROOT}/examples/" "${SRC_DIR}/examples/"
            fi
            if [ -d "${REPO_ROOT}/src" ] && [ -d "${SRC_DIR}/src" ]; then
                rsync -a --exclude='.git' --exclude='build' "${REPO_ROOT}/src/" "${SRC_DIR}/src/"
            fi
        fi

        if ! ninja -C "${BUILD_DIR}" "${BIN_NAME}" 2>/dev/null; then
            echo "⚙️ Reconfiguring with bitbake compile..."
            "${REPO_ROOT}/layers/poky/bitbake/bin/bitbake" -C compile "${RECIPE_NAME}"
        fi
    fi

    local elapsed=$(( ($(date +%s%N) - start_time) / 1000000 ))
    echo "✅ Cross-compilation finished in ${elapsed} ms: ${LOCAL_BIN}"
}

cmd_deploy() {
    echo "🚀 [2/3] Deploying to ${TARGET_USER}@${TARGET_IP}..."
    local start_time=$(date +%s%N)

    if [ ! -f "${LOCAL_BIN}" ]; then
        echo "❌ Binary not found at ${LOCAL_BIN}. Running build first..."
        cmd_build
    fi

    local base_bin="$(basename "${LOCAL_BIN}")"

    # 1. Stop existing process on target
    ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" "killall -9 ${base_bin} qtui_starter_template qtui_snake_game qtui_device_cockpit 2>/dev/null || true"

    # 2. SCP to /tmp/ then atomically move into /usr/bin/
    scp -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${LOCAL_BIN}" "${TARGET_USER}@${TARGET_IP}:/tmp/${base_bin}"

    ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" "mv -f /tmp/${base_bin} ${REMOTE_BIN} && chmod +x ${REMOTE_BIN}"

    # 3. If shared libraries deployment is requested:
    if [ "${DEPLOY_LIBS}" = "1" ] || [ "${ACTION}" = "deploy-libs" ]; then
        echo "📦 Deploying updated Qtui shared libraries..."
        local qml_dir="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0/build/qml/Qtui"
        if [ -d "${qml_dir}" ]; then
            tar -C "${qml_dir}" -czf - . 2>/dev/null | \
                ssh -o BatchMode=yes "${TARGET_USER}@${TARGET_IP}" 'tar -C /usr/lib/qml/Qtui -xzf -' 2>/dev/null || true
        fi
        for lib in $(find "${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui/0.2.0/build/src" -name "libQtui*.so" 2>/dev/null); do
            local lib_name="$(basename "${lib}")"
            scp -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
                "${lib}" "${TARGET_USER}@${TARGET_IP}:/tmp/${lib_name}" 2>/dev/null && \
            ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
                "${TARGET_USER}@${TARGET_IP}" "cp /tmp/${lib_name} /usr/lib/${lib_name}; cp /tmp/${lib_name} /usr/lib/qml/Qtui/Canvas/${lib_name} 2>/dev/null || true" 2>/dev/null || true
        done
    fi

    local elapsed=$(( ($(date +%s%N) - start_time) / 1000000 ))
    echo "✅ Deployed ${base_bin} -> ${TARGET_IP} in ${elapsed} ms"
}

cmd_run_tty1() {
    echo "🖥️ [3/3] Running '${APP_NAME}' on physical screen /dev/tty1..."

    if [ "${REMOTE_BIN}" = "/usr/bin/qtui_starter_template" ]; then
        ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
            "${TARGET_USER}@${TARGET_IP}" \
            "systemctl stop qtui-dev >/dev/null 2>&1 || true; \
             systemctl reset-failed qtui-dev >/dev/null 2>&1 || true; \
             /usr/bin/kbd_mode -u -C /dev/tty1 >/dev/null 2>&1 || true; \
             /usr/bin/setfont /usr/share/consolefonts/latarcyrheb-sun16.psfu.gz -C /dev/tty1 2>/dev/null || true; \
             printf '\033%%G' > /dev/tty1 2>/dev/null || true; \
             systemctl restart qtui-app"
    else
        ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
            "${TARGET_USER}@${TARGET_IP}" \
            "systemctl stop qtui-app qtui-dev >/dev/null 2>&1 || true; \
             systemctl reset-failed qtui-dev >/dev/null 2>&1 || true; \
             /usr/bin/kbd_mode -u -C /dev/tty1 >/dev/null 2>&1 || true; \
             /usr/bin/setfont /usr/share/consolefonts/latarcyrheb-sun16.psfu.gz -C /dev/tty1 2>/dev/null || true; \
             printf '\033%%G' > /dev/tty1 2>/dev/null || true; \
             systemd-run --unit=qtui-dev \
                -p StandardInput=tty \
                -p StandardOutput=tty \
                -p StandardError=journal \
                -p TTYPath=/dev/tty1 \
                -p Environment='TERM=linux' \
                -p Environment='COLORTERM=truecolor' \
                -p Environment='LANG=C.UTF-8' \
                -p Environment='LC_ALL=C.UTF-8' \
                -p Environment='QML_IMPORT_PATH=/usr/lib/qml:/usr/lib/qt6/qml' \
                -p Environment='QML2_IMPORT_PATH=/usr/lib/qml:/usr/lib/qt6/qml' \
                ${REMOTE_BIN}"
    fi
    echo "✨ Application '${BIN_NAME}' is now LIVE on /dev/tty1 with Unicode 16px font!"
}

cmd_run_ssh() {
    echo "🖥️ Launching interactive SSH session for '${APP_NAME}'..."
    ssh -t -o ConnectTimeout=5 -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" \
        "TERM=xterm-256color COLORTERM=truecolor LANG=C.UTF-8 QML_IMPORT_PATH=/usr/lib/qml:/usr/lib/qt6/qml ${REMOTE_BIN}"
}

cmd_restart_tty1() {
    echo "🔄 Restarting qtui-app on /dev/tty1..."
    ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" \
        "systemctl stop qtui-dev >/dev/null 2>&1 || true; \
         /usr/bin/kbd_mode -u -C /dev/tty1 >/dev/null 2>&1 || true; \
         /usr/bin/setfont /usr/share/consolefonts/latarcyrheb-sun16.psfu.gz -C /dev/tty1 2>/dev/null || true; \
         printf '\033%%G' > /dev/tty1 2>/dev/null || true; \
         systemctl restart qtui-app"
    echo "✅ qtui-app restarted on /dev/tty1 with high-DPI font"
}

cmd_stop_tty1() {
    echo "🛑 Stopping UI services on /dev/tty1..."
    ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" "systemctl stop qtui-app qtui-dev >/dev/null 2>&1 || true"
    echo "✅ Display /dev/tty1 is freed"
}

cmd_status() {
    echo "🔍 Querying target ${TARGET_USER}@${TARGET_IP}..."
    ssh -o ConnectTimeout=5 -o BatchMode=yes -o StrictHostKeyChecking=no \
        "${TARGET_USER}@${TARGET_IP}" \
        "printf 'Host: '; hostname; \
         printf 'Kernel: '; uname -r; \
         printf 'Active UI Service: '; systemctl is-active qtui-app qtui-dev 2>/dev/null | tr '\n' ' '; echo ''; \
         printf 'IP Address: '; ip -4 addr show mlan0 2>/dev/null | grep inet | awk '{print \$2}' || true"
}

case "${ACTION}" in
    dev)
        cmd_build
        cmd_deploy
        cmd_run_tty1
        ;;
    build)
        cmd_build
        ;;
    deploy)
        cmd_deploy
        ;;
    run-tty1|tty1)
        cmd_run_tty1
        ;;
    run-ssh|ssh)
        cmd_run_ssh
        ;;
    restart-tty1|restart)
        cmd_restart_tty1
        ;;
    stop-tty1|stop)
        cmd_stop_tty1
        ;;
    status)
        cmd_status
        ;;
    help|-h|--help)
        show_help
        ;;
    *)
        echo "❌ Unknown action: ${ACTION}"
        show_help
        exit 1
        ;;
esac
