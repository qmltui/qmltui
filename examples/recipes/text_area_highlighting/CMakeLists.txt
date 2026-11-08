qt_add_executable(qmltui_text_area_highlighting_demo main.cpp)

qt_add_qml_module(qmltui_text_area_highlighting_demo
    URI TextAreaHighlightingDemo
    VERSION 1.0
    SOURCES
        JsonHighlighter.h
        JsonHighlighter.cpp
    QML_FILES
        Main.qml
        QmlHighlighter.qml

    DEPENDENCIES TARGET qmltui::core
)

target_link_libraries(qmltui_text_area_highlighting_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_text_area_highlighting_demo)
endif()

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(qmltui_text_area_highlighting_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME text_area_highlighting_smoke TARGET qmltui_text_area_highlighting_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL)
    qmltui_add_scenario(NAME text_area_highlighting_demo TARGET qmltui_text_area_highlighting_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK REAL RECORD components/text_area_highlighting.cast)
endif()
