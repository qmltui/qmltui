# ==============================================================================
# Qtui Embedded ARM64 SDK Environment Setup
# Usage: source meta-qtui/scripts/environment-setup-arm64.sh
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

export SDKTARGETSYSROOT="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/recipe-sysroot"
export NATIVE_SYSROOT="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/recipe-sysroot-native"
export CMAKE_TOOLCHAIN_FILE="${REPO_ROOT}/build/tmp/work/cortexa55-poky-linux/qtui-starter-template/1.0.0/toolchain.cmake"

if [ ! -d "${SDKTARGETSYSROOT}" ]; then
    echo "⚠️ Warning: Target sysroot not found in ${SDKTARGETSYSROOT}"
    echo "💡 Run 'bitbake qtui-starter-template' to prepare sysroot artifacts."
fi

# Toolchain paths
export PATH="${NATIVE_SYSROOT}/usr/bin/aarch64-poky-linux:${NATIVE_SYSROOT}/usr/bin:${PATH}"

# Compiler and Linker
export CC="aarch64-poky-linux-gcc --sysroot=${SDKTARGETSYSROOT} -mcpu=cortex-a55+crypto"
export CXX="aarch64-poky-linux-g++ --sysroot=${SDKTARGETSYSROOT} -mcpu=cortex-a55+crypto"
export CPP="aarch64-poky-linux-gcc -E --sysroot=${SDKTARGETSYSROOT} -mcpu=cortex-a55+crypto"
export AS="aarch64-poky-linux-as"
export LD="aarch64-poky-linux-ld --sysroot=${SDKTARGETSYSROOT}"
export AR="aarch64-poky-linux-ar"
export NM="aarch64-poky-linux-nm"
export STRIP="aarch64-poky-linux-strip"
export OBJCOPY="aarch64-poky-linux-objcopy"
export OBJDUMP="aarch64-poky-linux-objdump"
export READELF="aarch64-poky-linux-readelf"

# Package Config & Qt Host Tools
export PKG_CONFIG_PATH="${SDKTARGETSYSROOT}/usr/lib/pkgconfig:${SDKTARGETSYSROOT}/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="${SDKTARGETSYSROOT}"
export QT_HOST_PATH="${NATIVE_SYSROOT}/usr"
export CMAKE_PREFIX_PATH="${SDKTARGETSYSROOT}/usr;${SDKTARGETSYSROOT}/usr/lib/cmake"

# Target IP default for deployment
export QTUI_TARGET_IP="${QTUI_TARGET_IP:-192.168.1.35}"

echo "✅ Qtui ARM64 Toolchain Environment Initialized!"
echo "   Target Architecture: ARMv8-A (aarch64 Cortex-A55)"
echo "   Target Sysroot:      ${SDKTARGETSYSROOT}"
echo "   Compiler:            aarch64-poky-linux-g++"
echo "   Qt6 Host Path:       ${QT_HOST_PATH}"
echo "   Cross CMake Tool:    ${REPO_ROOT}/meta-qtui/scripts/qtui-cross-cmake.sh"
echo "   Dev Deploy Tool:     ${REPO_ROOT}/meta-qtui/scripts/qtui-dev.sh"
