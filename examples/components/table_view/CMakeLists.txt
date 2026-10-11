qt_add_executable(qamui_table_view_demo main.cpp)

qt_add_qml_module(qamui_table_view_demo
    URI TableViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES ProcessProxy.h ProcessProxy.cpp
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_table_view_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_table_view_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME tableview_proxy_smoke TARGET qamui_table_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_proxy.js"
        COLUMNS 80 ROWS 24 LABELS AgentSmoke)
endif()
