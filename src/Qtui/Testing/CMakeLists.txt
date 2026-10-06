add_library(QtuiTesting
    TestRunner.h
    TestRunner.cpp
    MemoryTerminalBackend.h
    MemoryTerminalBackend.cpp
)

target_compile_definitions(QtuiTesting PRIVATE QTUI_TESTING_LIBRARY)
add_library(Qtui::Testing ALIAS QtuiTesting)
set_target_properties(QtuiTesting PROPERTIES EXPORT_NAME Testing)

target_include_directories(QtuiTesting PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui>
)

target_link_libraries(QtuiTesting
    PUBLIC
        Qt6::Qml
        Qtui::Core
    PRIVATE
        Qt6::Gui
)

# The auto-hook (TestRunner.cpp's file-scope TestRunnerAutoHook) sits in a
# translation unit that nothing references, so the linker drops it from the
# static archive unless the archive is force-loaded. Reference the namespaced
# target: it is an ALIAS in this build tree and an IMPORTED target for
# find_package() consumers, so the expression resolves in both. Using the
# unnamespaced name here would export a build-tree target name and break
# every consumer at generate time.
if(MSVC)
    target_link_options(QtuiTesting INTERFACE
        $<$<LINK_LANGUAGE:CXX>:/WHOLEARCHIVE:$<TARGET_FILE:Qtui::Testing>>
    )
elseif(APPLE)
    # ld64 has no --whole-archive; -force_load takes the archive path directly.
    target_link_options(QtuiTesting INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,-force_load,$<TARGET_FILE:Qtui::Testing>>
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(QtuiTesting INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,--whole-archive $<TARGET_FILE:Qtui::Testing> -Wl,--no-whole-archive>
    )
endif()

if(QTUI_ENABLE_TEST_RUNNER OR QTUI_ENABLE_TEST_DRIVER)
    install(TARGETS QtuiTesting
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    install(FILES TestRunner.h MemoryTerminalBackend.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/Testing"
    )
endif()
