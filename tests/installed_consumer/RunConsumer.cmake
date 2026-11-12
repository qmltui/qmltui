if(NOT DEFINED QMLTUI_BUILD_DIR OR NOT DEFINED CONSUMER_SOURCE_DIR
   OR NOT DEFINED CONSUMER_BINARY_DIR OR NOT DEFINED INSTALL_PREFIX)
    message(FATAL_ERROR "Installed consumer paths were not provided")
endif()

file(REMOVE_RECURSE "${CONSUMER_BINARY_DIR}" "${INSTALL_PREFIX}")

get_filename_component(consumer_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)

# Match the parent build's generator and configuration so the consumer binary
# lands where this script expects it (single-config layout).
set(consumer_generator_args)
if(DEFINED CONSUMER_GENERATOR AND NOT "${CONSUMER_GENERATOR}" STREQUAL "")
    list(APPEND consumer_generator_args -G "${CONSUMER_GENERATOR}")
endif()
if(DEFINED CONSUMER_BUILD_TYPE AND NOT "${CONSUMER_BUILD_TYPE}" STREQUAL "")
    list(APPEND consumer_generator_args "-DCMAKE_BUILD_TYPE=${CONSUMER_BUILD_TYPE}")
endif()
if(DEFINED CONSUMER_CXX_COMPILER AND NOT "${CONSUMER_CXX_COMPILER}" STREQUAL "")
    list(APPEND consumer_generator_args "-DCMAKE_CXX_COMPILER=${CONSUMER_CXX_COMPILER}")
endif()
if(DEFINED CONSUMER_CXX_FLAGS AND NOT "${CONSUMER_CXX_FLAGS}" STREQUAL "")
    list(APPEND consumer_generator_args "-DCMAKE_CXX_FLAGS=${CONSUMER_CXX_FLAGS}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${QMLTUI_BUILD_DIR}"
            --prefix "${INSTALL_PREFIX}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "QmlTui install failed:\n${install_output}\n${install_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
            -S "${CONSUMER_SOURCE_DIR}"
            -B "${CONSUMER_BINARY_DIR}"
            ${consumer_generator_args}
            # The Qt that built QmlTui must win over any other Qt reachable from
            # PATH (WSL exposes Windows installations, for example).
            "-DCMAKE_PREFIX_PATH=${INSTALL_PREFIX};${consumer_qt_prefix}"
            "-DQt6_DIR=${Qt6_DIR}"
            "-DQMLTUI_PUBLIC_HEADERS=${QMLTUI_BUILD_DIR}/qmltui_public_headers.h"
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_output
    ERROR_VARIABLE configure_error
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Consumer configure failed:\n${configure_output}\n${configure_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${CONSUMER_BINARY_DIR}"
    RESULT_VARIABLE build_result
    OUTPUT_VARIABLE build_output
    ERROR_VARIABLE build_error
)
if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Consumer build failed:\n${build_output}\n${build_error}")
endif()

get_filename_component(qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(runtime_library_path
    "${INSTALL_PREFIX}/lib:${qt_prefix}/lib:$ENV{LD_LIBRARY_PATH}")

set(consumer_executable "${CONSUMER_BINARY_DIR}/qmltui_installed_custom_item_consumer")
if(WIN32)
    string(APPEND consumer_executable ".exe")
    # DLLs are found through PATH on Windows; QmlTui installs its runtime next to
    # the QML modules. Set the environment directly: a PATH value holds
    # semicolons, which CMake would otherwise treat as list separators.
    file(GLOB_RECURSE consumer_dll_paths "${INSTALL_PREFIX}/*.dll")
    set(consumer_dll_dirs)
    foreach(dll IN LISTS consumer_dll_paths)
        get_filename_component(dll_dir "${dll}" DIRECTORY)
        list(APPEND consumer_dll_dirs "${dll_dir}")
    endforeach()
    list(REMOVE_DUPLICATES consumer_dll_dirs)
    file(TO_NATIVE_PATH "${qt_prefix}/bin" consumer_path)
    foreach(dir IN LISTS consumer_dll_dirs)
        file(TO_NATIVE_PATH "${dir}" dir_native)
        string(APPEND consumer_path ";${dir_native}")
    endforeach()
    set(ENV{PATH} "${consumer_path};$ENV{PATH}")
    execute_process(
        COMMAND "${consumer_executable}"
        RESULT_VARIABLE run_result
        OUTPUT_VARIABLE run_output
        ERROR_VARIABLE run_error
    )
else()
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
                "LD_LIBRARY_PATH=${runtime_library_path}"
                "DYLD_LIBRARY_PATH=${INSTALL_PREFIX}/lib:${qt_prefix}/lib:$ENV{DYLD_LIBRARY_PATH}"
                "${consumer_executable}"
        RESULT_VARIABLE run_result
        OUTPUT_VARIABLE run_output
        ERROR_VARIABLE run_error
    )
endif()
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Consumer execution failed (${run_result}):\n${run_output}\n${run_error}")
endif()
