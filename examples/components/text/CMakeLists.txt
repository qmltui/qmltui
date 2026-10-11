qt_add_executable(qamui_text main.cpp)

qt_add_qml_module(qamui_text
    URI TextDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_text PRIVATE Qt6::Core Qt6::Qml Qamui::Core)


if(TARGET Qamui::Runner)
    qt_add_executable(qamui_text_test main.cpp)
    qt_add_qml_module(qamui_text_test
        URI TextDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/TextDemo"
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES TARGET Qamui::Core
    )
    target_link_libraries(qamui_text_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Runner)
    qamui_add_scenario_test(
        NAME text_demo_spans
        TARGET qamui_text_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_spans.js"
        COLUMNS 80 ROWS 24
        LABELS AgentSmoke
    )
endif()
