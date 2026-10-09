qt_add_executable(qtui_tableview_demo main.cpp)

qt_add_qml_module(qtui_tableview_demo
    URI TableViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES ProcessProxy.h ProcessProxy.cpp
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_tableview_demo PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

if(TARGET Qtui::Runner)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiRunner.cmake")
    qt_add_executable(qtui_tableview_test main_test.cpp)
    qt_add_qml_module(qtui_tableview_test
        URI TableViewTest
        VERSION 1.0
        QML_FILES Main.qml
        SOURCES ProcessProxy.h ProcessProxy.cpp
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
    )
 target_link_libraries(qtui_tableview_test PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::Runner)
    qtui_add_scenario_test(NAME tableview_proxy_smoke TARGET qtui_tableview_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_proxy.js"
        COLUMNS 80 ROWS 24 LABELS AgentSmoke)
endif()
