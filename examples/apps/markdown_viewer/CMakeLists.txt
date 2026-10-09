find_package(Qt6 REQUIRED COMPONENTS Gui)
include(FetchContent)

set(BUILD_MD4QT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_MD4QT_BENCHMARK OFF CACHE BOOL "" FORCE)
set(BUILD_MD2HTML_APP OFF CACHE BOOL "" FORCE)
set(BUILD_MD2QDOC_APP OFF CACHE BOOL "" FORCE)
set(INSTALL_MD4QT OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    md4qt
    GIT_REPOSITORY https://github.com/KDE/md4qt.git
    GIT_TAG 1a878810adeeb534de4de395f4951edd54ab6072
)
FetchContent_MakeAvailable(md4qt)

qt_add_library(qtui_markdown_viewer_module STATIC)

qt_add_qml_module(qtui_markdown_viewer_module
    URI Qtui.Examples.MarkdownViewer
    VERSION 1.0
    SOURCES
        MdRole.h
        MdPalette.h
        MdPalette.cpp
        MdDocStyle.h
        MdDocStyle.cpp
        MdViewStyle.h
        MdViewStyle.cpp
        MdDoc.h
        MdDoc.cpp
        MdView.h
        MdView.cpp
        SyntaxHighlighter.h
        SyntaxHighlighter.cpp
    QML_FILES
        Main.qml
        Theme.qml
        ModernDocStyle.qml
        ClassicDocStyle.qml
        MinimalDocStyle.qml
        StyleEditor.qml
        HeaderBar.qml
        StatusBar.qml
        SearchBar.qml
    RESOURCES
        resources/modern_showcase.md
        resources/welcome.md
        resources/qtui_guide.md
        resources/cheatsheet.md
        resources/edge_cases.md
        test_markdown_viewer.js
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Controls
)

target_include_directories(qtui_markdown_viewer_module
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${md4qt_SOURCE_DIR}/src ${md4qt_SOURCE_DIR}
)

target_compile_definitions(qtui_markdown_viewer_module PRIVATE MD4QT_QT_SUPPORT)

target_link_libraries(qtui_markdown_viewer_module PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Gui
    Qtui::Core
    Qtui::Controls
    Qtui::Controls
    md4qt::md4qt
)

qt_add_executable(qtui_markdown_viewer main.cpp)
target_link_libraries(qtui_markdown_viewer PRIVATE
    Qt6::Core
    Qt6::Qml
    qtui_markdown_viewer_moduleplugin
)

if(QTUI_INSTALL_EXAMPLES)
    install(TARGETS qtui_markdown_viewer RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if(TARGET Qtui::TestRunner)
    qt_add_executable(qtui_markdown_viewer_test_runner main_test_runner.cpp)
    target_link_libraries(qtui_markdown_viewer_test_runner PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
        qtui_markdown_viewer_moduleplugin
    )

    if(QTUI_BUILD_TESTS)
        function(add_markdown_viewer_scenario name columns rows)
            add_test(NAME ${name} COMMAND
                "${CMAKE_COMMAND}"
                "-DTEST_EXECUTABLE=$<TARGET_FILE:qtui_markdown_viewer_test_runner>"
                "-DARTIFACT_DIR=${CMAKE_CURRENT_BINARY_DIR}/TestRunnerArtifacts/${name}"
                "-DEXPECT_EXIT=0"
                "-DPROCESS_TIMEOUT=20"
                "-DCOLUMNS=${columns}"
                "-DROWS=${rows}"
                "-DOPERATION_TIMEOUT_MS=2000"
                "-DSCENARIO_TIMEOUT_MS=15000"
                "-DQML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
                "-DEMBEDDED_SCENARIO=ON"
                "-P" "${PROJECT_SOURCE_DIR}/cmake/RunQtuiScenario.cmake"
            )
            set_tests_properties(${name} PROPERTIES
                TIMEOUT 23
                LABELS "TestRunner;AgentSmoke"
            )
        endfunction()
        add_markdown_viewer_scenario(markdown_viewer_scenario_wide 100 32)
        add_markdown_viewer_scenario(markdown_viewer_scenario_narrow 70 24)
    endif()
endif()

if(QTUI_BUILD_TESTS)
    qt_add_executable(test_markdown_viewer_document test_markdown_viewer.cpp)
    target_link_libraries(test_markdown_viewer_document PRIVATE
        Qt6::Core
        Qt6::Test
        Qtui::Core
        qtui_markdown_viewer_module
    )
    add_test(NAME markdown_viewer_document COMMAND test_markdown_viewer_document)
    set_tests_properties(markdown_viewer_document PROPERTIES LABELS AgentSmoke)
endif()
