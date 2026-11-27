# Runner relies on file-scope RunnerAutoHook static initialization via whole-archive
# link flags, so it is always built as a PIC static archive even when BUILD_SHARED_LIBS=ON.
add_library(QmlTuiRunner STATIC
    Runner.h
    Runner.cpp
    RunnerInput_p.h
    RunnerInput.cpp
    RunnerInteractive_p.h
    RunnerInteractive.cpp
    RunnerSnapshot_p.h
    RunnerSnapshot.cpp
    CastRecorder_p.h
    CastRecorder.cpp
    MemoryTerminalBackend.h
    MemoryTerminalBackend.cpp
)
set_target_properties(QmlTuiRunner PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Configuring the Runner target is what makes the scenario helpers
# (qmltui_add_scenario, qmltui_add_scenario_test, qmltui_scenario_editor_support)
# available: CMake functions are global once defined, and this directory is
# processed before examples/, tests/ and any FetchContent/add_subdirectory
# consumer's own directories. No other CMakeLists.txt in the tree includes the
# module; installed consumers get it from QmlTuiConfig.cmake. The editor-support
# helper also needs the source location of the scenario API declarations.
include("${PROJECT_SOURCE_DIR}/cmake/QmlTuiRunner.cmake")
set(QMLTUI_RUNNER_DECLARATIONS "${CMAKE_CURRENT_SOURCE_DIR}/qmltui-runner.d.ts"
    CACHE INTERNAL "Source location of the Runner scenario API declarations")

get_target_property(qmltui_runner_library_type QmlTuiRunner TYPE)
if(qmltui_runner_library_type STREQUAL "STATIC_LIBRARY")
    target_compile_definitions(QmlTuiRunner PUBLIC QMLTUI_RUNNER_STATIC)
else()
    target_compile_definitions(QmlTuiRunner PRIVATE QMLTUI_RUNNER_LIBRARY)
endif()
add_library(QmlTui::Runner ALIAS QmlTuiRunner)
set_target_properties(QmlTuiRunner PROPERTIES EXPORT_NAME Runner)

target_include_directories(QmlTuiRunner PUBLIC
    $<BUILD_INTERFACE:${qmltui_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(QmlTuiRunner PUBLIC
    QmlTui::Core
)

# The auto-hook (Runner.cpp's file-scope RunnerAutoHook) sits in a
# translation unit that nothing references, so the linker drops it from the
# static archive unless the archive is force-loaded. Reference the namespaced
# target: it is an ALIAS in this build tree and an IMPORTED target for
# find_package() consumers, so the expression resolves in both. Using the
# unnamespaced name here would export a build-tree target name and break
# every consumer at generate time.
if(MSVC)
    target_link_options(QmlTuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:/WHOLEARCHIVE:$<TARGET_FILE:QmlTui::Runner>>
    )
elseif(APPLE)
    # ld64 has no --whole-archive; -force_load takes the archive path directly.
    target_link_options(QmlTuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,-force_load,$<TARGET_FILE:QmlTui::Runner>>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(QmlTuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,--whole-archive $<TARGET_FILE:QmlTui::Runner> -Wl,--no-whole-archive>
    )
endif()

if(QMLTUI_ENABLE_RUNNER)
    install(TARGETS QmlTuiRunner
        EXPORT QmlTuiRunnerTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    qmltui_install_headers(QmlTui/Runner FILES Runner.h MemoryTerminalBackend.h)
    # Editor completion for scenario authors: referenced from a jsconfig.json,
    # not compiled, so it is not part of the public-header check.
    install(FILES qmltui-runner.d.ts
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/QmlTui/Runner")
endif()
