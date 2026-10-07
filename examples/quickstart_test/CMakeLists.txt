# In-tree build of the newcomer quickstart. The three files under
# ../quickstart/ are the tutorial and stay untouched; this directory compiles
# them against the current library so any API drift fails the build here, and
# adds a TestRunner scenario that exercises the rendered result.
set(quickstart_dir "${CMAKE_CURRENT_SOURCE_DIR}/../quickstart")
set_source_files_properties("${quickstart_dir}/Main.qml" PROPERTIES QT_RESOURCE_ALIAS Main.qml)

qt_add_executable(qtui_quickstart "${quickstart_dir}/main.cpp")
# NO_CACHEGEN: qmlcachegen derives an output directory from the QML file's
# relative path, and "../quickstart" is not a valid directory name on Windows.
qt_add_qml_module(qtui_quickstart
    URI HelloQtui
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES "${quickstart_dir}/Main.qml"
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)
target_link_libraries(qtui_quickstart PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)

if(TARGET Qtui::TestRunner)
    include("${Qtui_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
    # Same sources plus Qtui::TestRunner, whose auto-hook honours QTUI_SCRIPT.
    qt_add_executable(qtui_quickstart_test "${quickstart_dir}/main.cpp")
    qt_add_qml_module(qtui_quickstart_test
        URI HelloQtui
        VERSION 1.0
        NO_CACHEGEN
        QML_FILES "${quickstart_dir}/Main.qml"
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/HelloQtui"
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
    )
    target_link_libraries(qtui_quickstart_test PRIVATE
 Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::TestRunner)

    qtui_add_scenario_test(
        NAME quickstart_smoke
        TARGET qtui_quickstart_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_quickstart.js"
        LABELS AgentSmoke
    )
endif()
