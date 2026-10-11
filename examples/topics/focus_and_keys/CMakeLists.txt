qt_add_executable(qamui_focus_and_keys_demo main.cpp)

qt_add_qml_module(qamui_focus_and_keys_demo
    URI FocusAndKeys
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_focus_and_keys_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_focus_and_keys_demo_test main.cpp)
    qt_add_qml_module(qamui_focus_and_keys_demo_test
        URI FocusAndKeys
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/FocusAndKeys"
        VERSION 1.0
        QML_FILES
            Main.qml

        DEPENDENCIES TARGET Qamui::Core
    )
    target_link_libraries(qamui_focus_and_keys_demo_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Runner)
    qamui_add_scenario(NAME focus_and_keys_smoke TARGET qamui_focus_and_keys_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
