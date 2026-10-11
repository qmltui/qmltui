qt_add_executable(qamui_slider_demo main.cpp)

qt_add_qml_module(qamui_slider_demo
    URI SliderDemo
    VERSION 1.0
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_slider_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls)

if(TARGET Qamui::Runner)
    qt_add_executable(qamui_slider_demo_test main.cpp)
    qt_add_qml_module(qamui_slider_demo_test
        URI SliderDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/SliderDemo"
        VERSION 1.0
        QML_FILES
            Main.qml

     DEPENDENCIES TARGET Qamui::Core Qamui::Controls
    )
    target_link_libraries(qamui_slider_demo_test PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
    qamui_add_scenario(NAME slider_smoke TARGET qamui_slider_demo_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 120 ROWS 40 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
