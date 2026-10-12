qt_add_executable(qmltui_fluid_background_demo main.cpp)

qt_add_qml_module(qmltui_fluid_background_demo
    URI FluidBackgroundDemo
    VERSION 1.0
    SOURCES
        FluidBackground.h
        FluidBackground.cpp
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET qmltui::core
)

target_link_libraries(qmltui_fluid_background_demo PRIVATE Qt6::Core Qt6::Qml qmltui::core)

if(TARGET qmltui::runner)
    target_link_libraries(qmltui_fluid_background_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(NAME fluid_background_smoke TARGET qmltui_fluid_background_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
