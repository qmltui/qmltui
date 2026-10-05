if(NOT DEFINED QTUI_BUILD_DIR OR NOT DEFINED CONSUMER_SOURCE_DIR
   OR NOT DEFINED CONSUMER_BINARY_DIR OR NOT DEFINED INSTALL_PREFIX)
    message(FATAL_ERROR "Installed consumer paths were not provided")
endif()

file(REMOVE_RECURSE "${CONSUMER_BINARY_DIR}" "${INSTALL_PREFIX}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${QTUI_BUILD_DIR}"
            --prefix "${INSTALL_PREFIX}"
    RESULT_VARIABLE install_result
    OUTPUT_VARIABLE install_output
    ERROR_VARIABLE install_error
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Qtui install failed:\n${install_output}\n${install_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
            -S "${CONSUMER_SOURCE_DIR}"
            -B "${CONSUMER_BINARY_DIR}"
            "-DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}"
            "-DQt6_DIR=${Qt6_DIR}"
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

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
            "LD_LIBRARY_PATH=${runtime_library_path}"
            "DYLD_LIBRARY_PATH=${INSTALL_PREFIX}/lib:${qt_prefix}/lib:$ENV{DYLD_LIBRARY_PATH}"
            "${CONSUMER_BINARY_DIR}/qtui_installed_custom_item_consumer"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)
if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Consumer execution failed (${run_result}):\n${run_output}\n${run_error}")
endif()
