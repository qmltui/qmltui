add_library(QtuiTestRunner
    TestRunner.h
    TestRunner.cpp
    MemoryTerminalBackend.h
    MemoryTerminalBackend.cpp
)

target_compile_definitions(QtuiTestRunner PRIVATE QTUI_TESTRUNNER_LIBRARY)
add_library(Qtui::TestRunner ALIAS QtuiTestRunner)
set_target_properties(QtuiTestRunner PROPERTIES EXPORT_NAME TestRunner)

target_include_directories(QtuiTestRunner PUBLIC
    $<BUILD_INTERFACE:${Qtui_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(QtuiTestRunner PUBLIC
    Qt6::Qml
    Qtui::Core
)

# The auto-hook (TestRunner.cpp's file-scope TestRunnerAutoHook) sits in a
# translation unit that nothing references, so the linker drops it from the
# static archive unless the archive is force-loaded. Reference the namespaced
# target: it is an ALIAS in this build tree and an IMPORTED target for
# find_package() consumers, so the expression resolves in both. Using the
# unnamespaced name here would export a build-tree target name and break
# every consumer at generate time.
if(MSVC)
    target_link_options(QtuiTestRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:/WHOLEARCHIVE:$<TARGET_FILE:Qtui::TestRunner>>
    )
elseif(APPLE)
    # ld64 has no --whole-archive; -force_load takes the archive path directly.
    target_link_options(QtuiTestRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,-force_load,$<TARGET_FILE:Qtui::TestRunner>>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(QtuiTestRunner INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,--whole-archive $<TARGET_FILE:Qtui::TestRunner> -Wl,--no-whole-archive>
    )
endif()

if(QTUI_ENABLE_TEST_RUNNER)
    install(TARGETS QtuiTestRunner
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    qtui_install_headers(Qtui/TestRunner FILES TestRunner.h MemoryTerminalBackend.h)
endif()
