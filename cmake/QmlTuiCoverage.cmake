include_guard(GLOBAL)

# Source-based Clang code coverage for QmlTui libraries and tools.
# Enabled with -DQMLTUI_ENABLE_COVERAGE=ON when building with Clang.

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "QMLTUI_ENABLE_COVERAGE requires a Clang C++ compiler "
        "(CMAKE_CXX_COMPILER_ID is '${CMAKE_CXX_COMPILER_ID}'). "
        "Configure with -DCMAKE_CXX_COMPILER=clang++.")
endif()

find_program(QMLTUI_LLVM_PROFDATA NAMES llvm-profdata REQUIRED)
find_program(QMLTUI_LLVM_COV NAMES llvm-cov REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Instrument everything defined under src/QmlTui and tools/qmltui-runner, while
# leaving third-party libraries (utf8proc) untouched.
function(qmltui_enable_target_coverage target_name)
    if(NOT TARGET "${target_name}")
        return()
    endif()
    target_compile_options(${target_name} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-O0 -g -fprofile-instr-generate -fcoverage-mapping>
    )
    get_target_property(target_type ${target_name} TYPE)
    if(target_type STREQUAL "STATIC_LIBRARY")
        target_link_options(${target_name} PUBLIC
            $<$<LINK_LANGUAGE:CXX>:-fprofile-instr-generate>
        )
    else()
        target_link_options(${target_name} PRIVATE
            $<$<LINK_LANGUAGE:CXX>:-fprofile-instr-generate>
        )
    endif()
endfunction()

# Exclude generated moc/rcc/qmlcachegen files and any non-library paths.
set(QMLTUI_COVERAGE_IGNORE_REGEX
    "(_autogen/|/\\.rcc/|/qmlcache/|_[a-zA-Z0-9_]*qmltyperegistrations\\.cpp$|/tests/|/examples/|/3rdparty/)"
    CACHE STRING "Regular expression of paths excluded from llvm-cov reports"
)

function(qmltui_register_coverage_targets)
    set(instrumented_targets
        QmlTuiCore
        QmlTuiControls
        QmlTuiImage
        QmlTuiRunner
        qmltui-runner
    )

    set(cov_object_args)
    foreach(tgt IN LISTS instrumented_targets)
        if(TARGET "${tgt}")
            qmltui_enable_target_coverage("${tgt}")
            get_target_property(tgt_type "${tgt}" TYPE)
            if(NOT tgt_type STREQUAL "STATIC_LIBRARY")
                list(APPEND cov_object_args "$<TARGET_FILE:${tgt}>")
            endif()
        endif()
    endforeach()

    # QmlTuiRunner is always built as a static archive (for whole-archive
    # auto-hook registration), and llvm-cov requires linked binaries rather
    # than .a archives. qmltui_runner_fixture force-loads all of QmlTuiRunner,
    # while QMLTUI_COVERAGE_IGNORE_REGEX excludes tests/ from the report.
    if(TARGET QmlTuiRunner AND TARGET qmltui_runner_fixture)
        list(APPEND cov_object_args "$<TARGET_FILE:qmltui_runner_fixture>")
    endif()

    set(coverage_dir "${CMAKE_BINARY_DIR}/coverage")
    set(profdata_file "${coverage_dir}/qmltui.profdata")
    set(summary_txt "${coverage_dir}/summary.txt")
    set(summary_json "${coverage_dir}/summary.json")
    set(summary_md "${coverage_dir}/summary.md")
    set(badge_svg "${coverage_dir}/coverage.svg")
    set(lcov_info "${coverage_dir}/lcov.info")
    set(html_dir "${coverage_dir}/html")

    add_custom_target(coverage
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/coverage_report.py"
            --build-dir "${CMAKE_BINARY_DIR}"
            --source-dir "${PROJECT_SOURCE_DIR}"
            --coverage-dir "${coverage_dir}"
            --llvm-profdata "${QMLTUI_LLVM_PROFDATA}"
            --llvm-cov "${QMLTUI_LLVM_COV}"
            --ignore-regex "${QMLTUI_COVERAGE_IGNORE_REGEX}"
            --objects ${cov_object_args}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMENT "Running CTest suite and generating llvm-cov coverage report in ${coverage_dir}"
        USES_TERMINAL
        VERBATIM
    )
endfunction()
