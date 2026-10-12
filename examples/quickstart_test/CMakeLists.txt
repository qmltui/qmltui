# In-tree build of the newcomer quickstart. The three files under
# ../quickstart/ are the tutorial and stay untouched; this directory compiles
# them against the current library so any API drift fails the build here, and
# adds a Runner scenario that exercises the rendered result.
set(quickstart_dir "${CMAKE_CURRENT_SOURCE_DIR}/../quickstart")
set_source_files_properties("${quickstart_dir}/Main.qml" PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_executable(qmltui_quickstart "${quickstart_dir}/main.cpp")
# NO_CACHEGEN: qmlcachegen derives an output directory from the QML file's
# relative path, and "../quickstart" is not a valid directory name on Windows.
qt_add_qml_module(qmltui_quickstart
    URI HelloQmlTui
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES "${quickstart_dir}/Main.qml"
 DEPENDENCIES TARGET qmltui::core qmltui::controls
)
target_link_libraries(qmltui_quickstart PRIVATE Qt6::Core Qt6::Qml qmltui::core qmltui::controls)

if(TARGET qmltui::runner)
    # Same sources plus qmltui::runner, whose auto-hook honours QMLTUI_RUNNER_SCRIPT.
    qt_add_executable(qmltui_quickstart_test "${quickstart_dir}/main.cpp")
    qt_add_qml_module(qmltui_quickstart_test
        URI HelloQmlTui
        VERSION 1.0
        NO_CACHEGEN
        QML_FILES "${quickstart_dir}/Main.qml"
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/HelloQmlTui"
 DEPENDENCIES TARGET qmltui::core qmltui::controls
    )
    target_link_libraries(qmltui_quickstart_test PRIVATE
 Qt6::Core Qt6::Qml qmltui::core qmltui::controls qmltui::runner)

    qmltui_add_scenario_test(
        NAME quickstart_smoke
        TARGET qmltui_quickstart_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_quickstart.js"
        LABELS AgentSmoke

    )
endif()
