# The QML module is owned by each executable (not a shared static library) so
# Qt records the build tree's QML import path for it and `import Qtui`
# resolves when the binary is launched from Qt Creator or a shell.
qt_add_executable(qtui_lazy_model main.cpp)
qt_add_qml_module(qtui_lazy_model
    URI LazyModelDemo
    VERSION 1.0
    QML_FILES Main.qml
    SOURCES LazyDirectoryModel.h LazyDirectoryModel.cpp
    DEPENDENCIES TARGET Qtui::Core
)
target_link_libraries(qtui_lazy_model PRIVATE Qt6::Core Qt6::Qml Qtui::Core)

if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    qt_add_executable(qtui_lazy_model_test main.cpp)
    qt_add_qml_module(qtui_lazy_model_test
        URI LazyModelDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/LazyModelDemo"
        VERSION 1.0
        QML_FILES Main.qml
        SOURCES LazyDirectoryModel.h LazyDirectoryModel.cpp
        DEPENDENCIES TARGET Qtui::Core
    )
    target_link_libraries(qtui_lazy_model_test PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Runner)
    qtui_add_scenario_test(
        NAME lazy_model_smoke
        TARGET qtui_lazy_model_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_lazy_model.js"
        COLUMNS 80
        ROWS 24
        LABELS AgentSmoke
    )
endif()
