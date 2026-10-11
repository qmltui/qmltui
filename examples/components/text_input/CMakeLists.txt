qt_add_executable(qamui_text_input main.cpp)

qt_add_qml_module(qamui_text_input
    URI TextInputDemo
    VERSION 1.0
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qamui::Core
)

target_link_libraries(qamui_text_input PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(QAMUI_BUILD_TESTS)
    target_link_libraries(qamui_text_input PRIVATE Qamui::Runner)
endif()

