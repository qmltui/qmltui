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

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(qtui_starter_template_module STATIC)

qt_add_qml_module(qtui_starter_template_module
    URI StarterTemplateApp
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    SOURCES
        WifiManager.h
        WifiManager.cpp
    QML_FILES
        StatusBadge.qml
        Main.qml
        Theme.qml
        NetworkPage.qml
        AboutPage.qml
        SignalMeter.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Controls
)

target_link_libraries(qtui_starter_template_module PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Network
    Qtui::Core
    Qtui::Controls
    Qtui::Controls
)

# QML module libraries do not make their runtime plugins build dependencies.
# Keep focused starter-only builds runnable without building every example.
foreach(plugin_target IN ITEMS
        QtuiCoreplugin QtuiControlsplugin QtuiControlsplugin
 Qtui::CorePlugin Qtui::ControlsPlugin Qtui::ControlsPlugin)
    if(TARGET ${plugin_target})
        add_dependencies(qtui_starter_template_module ${plugin_target})
    endif()
endforeach()

qt_add_executable(qtui_starter_template main.cpp)
target_link_libraries(qtui_starter_template PRIVATE
    Qt6::Core
    Qt6::Qml
    qtui_starter_template_moduleplugin
)

install(TARGETS qtui_starter_template
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(TARGET Qtui::TestRunner)
    qt_add_executable(qtui_starter_template_test main_test.cpp)
    target_link_libraries(qtui_starter_template_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
        qtui_starter_template_moduleplugin
    )

    qt_add_executable(qtui_starter_template_benchmark main_benchmark.cpp)
    target_link_libraries(qtui_starter_template_benchmark PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
        qtui_starter_template_moduleplugin
    )

    install(TARGETS qtui_starter_template_benchmark
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    if(QTUI_BUILD_TESTS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
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
            qt_add_executable(test_starter_wifi_manager test_wifi_manager.cpp)
            target_link_libraries(test_starter_wifi_manager PRIVATE
                Qt6::Core
                Qt6::Qml
                Qt6::Test
                qtui_starter_template_module
            )
            add_test(NAME starter_template_wifi_manager COMMAND test_starter_wifi_manager)
            set_tests_properties(starter_template_wifi_manager PROPERTIES LABELS AgentSmoke)
        endif()
    endif()
endif()
