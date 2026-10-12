qt_add_executable(qmltui_loader_demo main.cpp)

qt_add_qml_module(qmltui_loader_demo
    URI LoaderDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET qmltui::core qmltui::controls
)

target_link_libraries(qmltui_loader_demo PRIVATE
    Qt6::Core
    Qt6::Qml
    qmltui::core
    qmltui::controls
)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_loader_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME loader_smoke TARGET qmltui_loader_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qmltui_add_scenario(NAME loader_demo TARGET qmltui_loader_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/loader.cast LABELS Cast)
endif()
