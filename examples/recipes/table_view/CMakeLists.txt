qt_add_executable(qmltui_table_view_demo main.cpp)

qt_add_qml_module(qmltui_table_view_demo
    URI TableViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES ProcessProxy.h ProcessProxy.cpp
    DEPENDENCIES TARGET qmltui::core TARGET qmltui::controls
)

target_link_libraries(qmltui_table_view_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_table_view_demo)
endif()

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(qmltui_table_view_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME tableview_proxy_smoke TARGET qmltui_table_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_proxy.js"
        COLUMNS 80 ROWS 24)
endif()
