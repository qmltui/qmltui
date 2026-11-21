qt_add_executable(qmltui_table_view_demo main.cpp)

qt_add_qml_module(qmltui_table_view_demo
    URI TableViewDemo
    QML_FILES
        Main.qml
    SOURCES ProcessProxy.h ProcessProxy.cpp
    DEPENDENCIES TARGET QmlTui::Core TARGET QmlTui::Controls
)

target_link_libraries(qmltui_table_view_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core QmlTui::Controls)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_table_view_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_table_view_demo PRIVATE QmlTui::Runner)
    qmltui_add_scenario(NAME tableview_proxy_smoke TARGET qmltui_table_view_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_proxy.js"
        COLUMNS 80 ROWS 24)
endif()
