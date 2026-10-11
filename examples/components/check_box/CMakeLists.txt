qt_add_executable(qamui_check_box_demo main.cpp)

qt_add_qml_module(qamui_check_box_demo
    URI CheckBoxDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_check_box_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_check_box_demo_test main.cpp)
    qt_add_qml_module(qamui_check_box_demo_test
        URI CheckBoxDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/CheckBoxDemo"
        VERSION 1.0
        QML_FILES
            Main.qml

     DEPENDENCIES TARGET Qamui::Core Qamui::Controls
    )
    target_link_libraries(qamui_check_box_demo_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario(NAME check_box_smoke TARGET qamui_check_box_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
