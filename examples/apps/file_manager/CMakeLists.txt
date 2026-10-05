set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(qtui_file_manager main.cpp)

qt_add_qml_module(qtui_file_manager
    URI Qtui.Examples.FileManager
    VERSION 1.0
    SOURCES
        FileManagerBackend.h
        FileManagerBackend.cpp
    QML_FILES
        Main.qml
        Theme.qml
        HeaderBar.qml
        StatusBar.qml
        ParentColumn.qml
        CurrentColumn.qml
        PreviewColumn.qml
        HelpDialog.qml
        InputDialog.qml
        ConfirmDialog.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
        TARGET Qtui::Extras
        TARGET Qtui::TerminalHost
)

target_link_libraries(qtui_file_manager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
    Qtui::Extras
    Qtui::TerminalHost
)

if(TARGET Qtui::Testing)
    target_link_libraries(qtui_file_manager PRIVATE Qtui::Testing)
endif()

if(QTUI_INSTALL_EXAMPLES)
    install(TARGETS qtui_file_manager RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if(TARGET Qtui::Testing AND QTUI_BUILD_TESTS)
    include("${PROJECT_SOURCE_DIR}/cmake/QtuiTesting.cmake")
    # idle_budget asserts the application does not repaint while nothing changes.
    foreach(scenario navigation preview search actions idle_budget)
        qtui_add_scenario_test(
            NAME file_manager_${scenario}
            TARGET qtui_file_manager
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_${scenario}.js"
            COLUMNS 100
            ROWS 30
            LABELS AgentSmoke
        )
    endforeach()
endif()
