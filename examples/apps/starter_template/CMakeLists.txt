if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(qtui_starter_template LANGUAGES CXX)

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    find_package(Qt6 REQUIRED COMPONENTS Core Qml Network)
    find_package(Qtui REQUIRED)

    qt_standard_project_setup()
endif()

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(qtui_starter_template main.cpp)

qt_add_qml_module(qtui_starter_template
    URI StarterTemplateApp
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    SOURCES
        WifiManager.h
        WifiManager.cpp
    QML_FILES
        Main.qml
        Theme.qml
        NetworkPage.qml
        AboutPage.qml
        SignalMeter.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Extras
)

target_link_libraries(qtui_starter_template PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Network
    Qtui::Core
    Qtui::Controls
    Qtui::Extras
)

install(TARGETS qtui_starter_template
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

# Scenario runner: when Qtui::Testing is linked, the application executes
# QTUI_SCRIPT/QTUI_EVAL scenarios against the live UI.
if(TARGET Qtui::Testing)
    target_link_libraries(qtui_starter_template PRIVATE Qtui::Testing)
endif()

if(TARGET Qtui::Testing AND QTUI_BUILD_TESTS AND NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTesting.cmake")
    qtui_add_scenario_test(
        NAME starter_template_setup
        TARGET qtui_starter_template
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_setup.js"
        COLUMNS 120
        ROWS 36
        LABELS AgentSmoke
    )
endif()
