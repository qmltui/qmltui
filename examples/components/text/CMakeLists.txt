qt_add_executable(qtui_text main.cpp)

qt_add_qml_module(qtui_text
    URI TextDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core
)

target_link_libraries(qtui_text PRIVATE Qt6::Core Qt6::Qml Qtui::Core)


if(TARGET Qtui::TestRunner)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    qt_add_executable(qtui_text_test main_test.cpp)
    qt_add_qml_module(qtui_text_test
        URI TextDemoTest
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qtui::Core
    )
    target_link_libraries(qtui_text_test PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::TestRunner)
    qtui_add_scenario_test(
        NAME text_demo_spans
        TARGET qtui_text_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_spans.js"
        COLUMNS 80 ROWS 24
        LABELS AgentSmoke
    )
endif()
