qt_add_executable(qamui_big_text_demo main.cpp)

# BigText is a recipe for a custom C++ item: it is compiled into this example's
# own QML module rather than shipped by the library.
qt_add_qml_module(qamui_big_text_demo
    URI BigTextDemo
    VERSION 1.0
    SOURCES
        BigText.h
        BigText.cpp
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_big_text_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_big_text_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME big_text_smoke TARGET qamui_big_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
    qamui_add_scenario(NAME big_text_demo TARGET qamui_big_text_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_demo.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL RECORD components/big_text.cast LABELS Cast)
endif()
