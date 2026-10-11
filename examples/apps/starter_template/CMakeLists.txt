if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(qamui_starter_template LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Qml Network)
    find_package(Qamui REQUIRED)

    include(GNUInstallDirs)
    qt_standard_project_setup()
endif()
find_package(Qt6 REQUIRED COMPONENTS Network)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Each executable owns the QML module (as table_view and cyber_audio do) so Qt
# writes the build tree's QML import path into a qt.conf beside it and
# `import Qamui.Controls` resolves when the binary is launched from Qt Creator
# or a shell. A shared static module library would not get that treatment, and
# since no C++ symbol of Qamui::Controls is referenced the linker would drop the
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
    # An optional fourth argument gives the module its own output directory, so
    # a second executable can compile the same URI without colliding on disk.
    set(module_output_directory)
    if(ARGC GREATER 3)
        set(module_output_directory OUTPUT_DIRECTORY "${ARGV3}")
    endif()
    qt_add_executable(${target} ${main_source})
    qt_add_qml_module(${target}
        URI ${uri}
        ${module_output_directory}
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        SOURCES ${starter_template_sources}
        QML_FILES ${starter_template_qml_files}
        DEPENDENCIES
            TARGET Qamui::Core
            TARGET Qamui::Controls
    )
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Network
        Qamui::Core
        Qamui::Controls
    )

    # QML module dependencies do not make their runtime plugins build
    # dependencies. Keep focused starter-only builds runnable without building
    # every example.
    foreach(plugin_target IN ITEMS
            QamuiCoreplugin QamuiControlsplugin Qamui::CorePlugin Qamui::ControlsPlugin)
        if(TARGET ${plugin_target})
            add_dependencies(${target} ${plugin_target})
        endif()
    endforeach()
endfunction()

starter_template_add_executable(qamui_starter_template StarterTemplateApp main.cpp)

install(TARGETS qamui_starter_template
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

if(TARGET Qamui::Runner)
    starter_template_add_executable(qamui_starter_template_test StarterTemplateApp main_test.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/qml_test/StarterTemplateApp")
    target_link_libraries(qamui_starter_template_test PRIVATE Qamui::Runner)

    starter_template_add_executable(qamui_starter_template_benchmark StarterTemplateApp main_benchmark.cpp
        "${CMAKE_CURRENT_BINARY_DIR}/qml_benchmark/StarterTemplateApp")
    target_link_libraries(qamui_starter_template_benchmark PRIVATE Qamui::Runner)

    install(TARGETS qamui_starter_template_benchmark
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    if(QAMUI_BUILD_TESTS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        qamui_add_scenario_test(
            NAME starter_template_setup
            TARGET qamui_starter_template_test
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_setup.js"
            COLUMNS 120
            ROWS 36
            LABELS AgentSmoke
        )
        qamui_add_scenario_test(
            NAME starter_template_latency
            TARGET qamui_starter_template_benchmark
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
