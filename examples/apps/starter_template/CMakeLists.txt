if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(qtui_starter_template LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Qml Network)
    find_package(Qtui REQUIRED)

    include(GNUInstallDirs)
    qt_standard_project_setup()
endif()
find_package(Qt6 REQUIRED COMPONENTS Network)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Each executable owns the QML module (as table_view and cyber_audio do) so Qt
# writes the build tree's QML import path into a qt.conf beside it and
# `import Qtui.Controls` resolves when the binary is launched from Qt Creator
# or a shell. A shared static module library would not get that treatment, and
# since no C++ symbol of Qtui::Controls is referenced the linker would drop the
# library that carries the module's qmldir.
set(starter_template_sources
    WifiManager.h
    WifiManager.cpp
)
set(starter_template_qml_files
    Main.qml
    Theme.qml
    StatusBadge.qml
    NetworkPage.qml
    AboutPage.qml
    SignalMeter.qml
)

# Adds an executable that compiles the module under its own URI.
function(starter_template_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    qt_add_qml_module(${target}
        URI ${uri}
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        SOURCES ${starter_template_sources}
        QML_FILES ${starter_template_qml_files}
        DEPENDENCIES
            TARGET Qtui::Core
            TARGET Qtui::Controls
    )
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Network
        Qtui::Core
        Qtui::Controls
    )

    # QML module dependencies do not make their runtime plugins build
    # dependencies. Keep focused starter-only builds runnable without building
    # every example.
    foreach(plugin_target IN ITEMS
            QtuiCoreplugin QtuiControlsplugin Qtui::CorePlugin Qtui::ControlsPlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

starter_template_add_executable(qtui_starter_template StarterTemplateApp main.cpp)

install(TARGETS qtui_starter_template
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(TARGET Qtui::Runner)
    starter_template_add_executable(qtui_starter_template_test StarterTemplateAppTest main_test.cpp)
    target_link_libraries(qtui_starter_template_test PRIVATE Qtui::Runner)

    starter_template_add_executable(qtui_starter_template_benchmark StarterTemplateAppBenchmark main_benchmark.cpp)
    target_link_libraries(qtui_starter_template_benchmark PRIVATE Qtui::Runner)

    install(TARGETS qtui_starter_template_benchmark
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    if(QTUI_BUILD_TESTS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        include("${PROJECT_SOURCE_DIR}/cmake/QtuiRunner.cmake")
        qtui_add_scenario_test(
            NAME starter_template_setup
            TARGET qtui_starter_template_test
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_setup.js"
            COLUMNS 120
            ROWS 36
            LABELS AgentSmoke
        )
        qtui_add_scenario_test(
            NAME starter_template_latency
            TARGET qtui_starter_template_benchmark
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_latency.js"
            COLUMNS 120
            ROWS 36
            LABELS AgentFull
        )

        if(UNIX)   # the fake `nmcli`/`iw` fixtures are shell scripts
            qt_add_executable(test_starter_wifi_manager
                test_wifi_manager.cpp
                ${starter_template_sources}
            )
            target_include_directories(test_starter_wifi_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
            target_link_libraries(test_starter_wifi_manager PRIVATE
                Qt6::Core
                Qt6::Qml
                Qt6::Network
                Qt6::Test
            )
            add_test(NAME starter_template_wifi_manager COMMAND test_starter_wifi_manager)
            set_tests_properties(starter_template_wifi_manager PROPERTIES LABELS AgentSmoke)
        endif()
    endif()
endif()
