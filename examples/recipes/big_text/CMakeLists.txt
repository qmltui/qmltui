qt_add_executable(qmltui_big_text_demo main.cpp)

# BigText is a recipe for a custom C++ item: it is compiled into this example's
# own QML module rather than shipped by the library.
qt_add_qml_module(qmltui_big_text_demo
    URI BigTextDemo
    SOURCES
        BigText.h
        BigText.cpp
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET QmlTui::Core
)

target_link_libraries(qmltui_big_text_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_big_text_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_big_text_demo PRIVATE QmlTui::Runner)
    qmltui_add_scenario(NAME big_text_smoke TARGET qmltui_big_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL)
    qmltui_add_scenario(NAME big_text_demo TARGET qmltui_big_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK REAL RECORD components/big_text.cast)
endif()
