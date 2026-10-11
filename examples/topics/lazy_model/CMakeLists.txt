# The QML module is owned by each executable (not a shared static library) so
# Qt records the build tree's QML import path for it and `import Qamui`
# resolves when the binary is launched from Qt Creator or a shell.
qt_add_executable(qamui_lazy_model_demo main.cpp)
qt_add_qml_module(qamui_lazy_model_demo
    URI LazyModelDemo
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES LazyDirectoryModel.h LazyDirectoryModel.cpp
    DEPENDENCIES TARGET Qamui::Core
)
target_link_libraries(qamui_lazy_model_demo PRIVATE Qt6::Core Qt6::Qml Qamui::Core)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_lazy_model_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME lazy_model_smoke
        TARGET qamui_lazy_model_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_lazy_model.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
