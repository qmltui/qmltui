qt_add_library(qtui_lazy_model_module STATIC)

qt_add_qml_module(qtui_lazy_model_module
    URI LazyModelDemo
    VERSION 1.0
    SOURCES
        LazyDirectoryModel.h
        LazyDirectoryModel.cpp
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_lazy_model_module PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

qt_add_executable(qtui_lazy_model main.cpp)
target_link_libraries(qtui_lazy_model PRIVATE Qt6::Core Qt6::Qml Qtui::Core qtui_lazy_model_module)

if(TARGET Qtui::TestRunner AND QTUI_BUILD_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qt_add_executable(qtui_lazy_model_test main.cpp)
    target_link_libraries(qtui_lazy_model_test PRIVATE
        Qt6::Core Qt6::Qml Qtui::Core Qtui::TestRunner qtui_lazy_model_module)
    qtui_add_scenario_test(
        NAME lazy_model_smoke
        TARGET qtui_lazy_model_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_lazy_model.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
