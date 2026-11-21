# The QML module is owned by each executable (not a shared static library) so
# Qt records the build tree's QML import path for it and `import QmlTui`
# resolves when the binary is launched from Qt Creator or a shell.
qt_add_executable(qmltui_lazy_model_demo main.cpp)
qt_add_qml_module(qmltui_lazy_model_demo
    URI LazyModelDemo
    QML_FILES Main.qml
    SOURCES LazyDirectoryModel.h LazyDirectoryModel.cpp
    DEPENDENCIES TARGET QmlTui::Core
)
target_link_libraries(qmltui_lazy_model_demo PRIVATE Qt6::Core Qt6::Qml QmlTui::Core)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_lazy_model_demo)
endif()

if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_lazy_model_demo PRIVATE QmlTui::Runner)
    qmltui_add_scenario(
        NAME lazy_model_smoke
        TARGET qmltui_lazy_model_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_lazy_model.js"
        COLUMNS 80
        ROWS 24
    )
endif()
