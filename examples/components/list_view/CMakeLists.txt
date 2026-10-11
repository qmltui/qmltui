qt_add_executable(qamui_list_view_demo main.cpp)

qt_add_qml_module(qamui_list_view_demo
    URI ListViewDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_list_view_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_list_view_demo_test main.cpp)
    qt_add_qml_module(qamui_list_view_demo_test
        URI ListViewDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/ListViewDemo"
        VERSION 1.0
        QML_FILES
            Main.qml

        DEPENDENCIES TARGET Qamui::Core
    )
    target_link_libraries(qamui_list_view_demo_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Runner)
    qamui_add_scenario(NAME list_view_smoke TARGET qamui_list_view_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
