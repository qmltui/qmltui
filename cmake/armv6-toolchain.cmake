set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(SYSROOT_PATH "$ENV{HOME}/rpi-sysroot")
set(CMAKE_SYSROOT ${SYSROOT_PATH})

# Explicit full host paths to x86_64 cross-compilers
set(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabihf-g++)

set(ARMV6_FLAGS "-marm -march=armv6zk -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -B${SYSROOT_PATH}/usr/lib/gcc/arm-linux-gnueabihf/14 -B${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf")
set(CMAKE_C_FLAGS "${ARMV6_FLAGS}" CACHE STRING "C flags" FORCE)
set(CMAKE_CXX_FLAGS "${ARMV6_FLAGS}" CACHE STRING "C++ flags" FORCE)

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_C_FLAGS CMAKE_CXX_FLAGS CMAKE_SYSROOT)

set(CMAKE_FIND_ROOT_PATH ${SYSROOT_PATH})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

list(APPEND CMAKE_PREFIX_PATH
    "${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf/cmake"
    "${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf"
    "${SYSROOT_PATH}/usr"
)

set(QT_HOST_PATH "/usr" CACHE PATH "Host Qt installation" FORCE)
set(QT_NO_PACKAGE_VERSION_CHECK TRUE)

# Avoid Ninja generator RPATH relinking error when cross-compiling
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Ensure cross-linker uses sysroot libraries
set(RPATH_LINK_FLAGS "-B${SYSROOT_PATH}/usr/lib/gcc/arm-linux-gnueabihf/14 -B${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf -L${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf -L${SYSROOT_PATH}/lib/arm-linux-gnueabihf -Wl,-rpath-link,${SYSROOT_PATH}/lib/arm-linux-gnueabihf -Wl,-rpath-link,${SYSROOT_PATH}/usr/lib/arm-linux-gnueabihf")
set(CMAKE_EXE_LINKER_FLAGS "${RPATH_LINK_FLAGS}" CACHE STRING "Linker flags" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS "${RPATH_LINK_FLAGS}" CACHE STRING "Linker flags" FORCE)
set(CMAKE_MODULE_LINKER_FLAGS "${RPATH_LINK_FLAGS}" CACHE STRING "Linker flags" FORCE)