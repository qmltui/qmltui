add_library(QtuiRunner
    Runner.h
    Runner.cpp
    MemoryTerminalBackend.h
    MemoryTerminalBackend.cpp
)

# The scenario helpers (qtui_add_scenario, qtui_scenario_editor_support) must
# be available to FetchContent/add_subdirectory consumers even when nothing in
# this build includes them, and the editor-support helper needs the source
# location of the scenario API declarations.
include("${PROJECT_SOURCE_DIR}/cmake/QtuiRunner.cmake")
set(QTUI_RUNNER_DECLARATIONS "${CMAKE_CURRENT_SOURCE_DIR}/qtui-runner.d.ts"
    CACHE INTERNAL "Source location of the Runner scenario API declarations")

target_compile_definitions(QtuiRunner PRIVATE QTUI_RUNNER_LIBRARY)
add_library(Qtui::Runner ALIAS QtuiRunner)
set_target_properties(QtuiRunner PROPERTIES EXPORT_NAME Runner)

target_include_directories(QtuiRunner PUBLIC
    $<BUILD_INTERFACE:${Qtui_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(QtuiRunner PUBLIC
    Qt6::Qml
    Qtui::Core
)

# The auto-hook (Runner.cpp's file-scope RunnerAutoHook) sits in a
# translation unit that nothing references, so the linker drops it from the
# static archive unless the archive is force-loaded. Reference the namespaced
# target: it is an ALIAS in this build tree and an IMPORTED target for
# find_package() consumers, so the expression resolves in both. Using the
# unnamespaced name here would export a build-tree target name and break
# every consumer at generate time.
if(MSVC)
    target_link_options(QtuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:/WHOLEARCHIVE:$<TARGET_FILE:Qtui::Runner>>
    )
elseif(APPLE)
    # ld64 has no --whole-archive; -force_load takes the archive path directly.
    target_link_options(QtuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,-force_load,$<TARGET_FILE:Qtui::Runner>>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(QtuiRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,--whole-archive $<TARGET_FILE:Qtui::Runner> -Wl,--no-whole-archive>
    )
endif()

if(QTUI_ENABLE_RUNNER)
    install(TARGETS QtuiRunner
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    qtui_install_headers(Qtui/Runner FILES Runner.h MemoryTerminalBackend.h)
    # Editor completion for scenario authors: referenced from a jsconfig.json,
    # not compiled, so it is not part of the public-header check.
    install(FILES qtui-runner.d.ts
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/Qtui/Runner")
endif()
