qt_add_executable(qamui_fluid_background_demo main.cpp)

qt_add_qml_module(qamui_fluid_background_demo
    URI FluidBackgroundDemo
    VERSION 1.0
    SOURCES
        FluidBackground.h
        FluidBackground.cpp
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_fluid_background_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_fluid_background_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(NAME fluid_background_smoke TARGET qamui_fluid_background_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 80 ROWS 24 CLOCK VIRTUAL LABELS AgentSmoke)
endif()
