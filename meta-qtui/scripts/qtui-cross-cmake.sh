#!/usr/bin/env bash
# ==============================================================================
# Qtui Cross-CMake Tool
# Cross-compiles ANY standalone CMake application (inside or outside Qtui repo)
# targeting ARM64 Cortex-A55 Linux with Qt 6.8 + Qtui.
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

SYSROOT="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/recipe-sysroot"
NATIVE_SYSROOT="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/recipe-sysroot-native"
TOOLCHAIN_FILE="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/toolchain.cmake"

if [ ! -d "${SYSROOT}" ] || [ ! -f "${TOOLCHAIN_FILE}" ]; then
    echo "❌ Cross toolchain sysroot not found in ${SYSROOT}"
    echo "💡 Run 'bitbake qtui-starter-template' once to prepare the SDK sysroot."
    exit 1
fi

export PATH="${NATIVE_SYSROOT}/usr/bin/aarch64-poky-linux:${NATIVE_SYSROOT}/usr/bin:${PATH}"
export PKG_CONFIG_PATH="${SYSROOT}/usr/lib/pkgconfig:${SYSROOT}/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${SYSROOT}"
export QT_HOST_PATH="${NATIVE_SYSROOT}/usr"

exec "${NATIVE_SYSROOT}/usr/bin/cmake" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
    -DCMAKE_PREFIX_PATH="${SYSROOT}/usr;${SYSROOT}/usr/lib/cmake" \
    -DCMAKE_SYSROOT="${SYSROOT}" \
    -DQT_HOST_PATH="${NATIVE_SYSROOT}/usr" \
    "$@"
