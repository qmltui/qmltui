set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(qamui_file_manager main.cpp)

qt_add_qml_module(qamui_file_manager
    URI Qamui.Examples.FileManager
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
        TARGET Qamui::Core
        TARGET Qamui::Controls
        TARGET Qamui::Controls
        TARGET Qamui::TerminalHost
)

target_link_libraries(qamui_file_manager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
    Qamui::Controls
    Qamui::Controls
    Qamui::TerminalHost
)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_file_manager PRIVATE Qamui::Runner)
endif()

if(QAMUI_INSTALL_EXAMPLES)
    install(TARGETS qamui_file_manager RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if(TARGET Qamui::Runner AND QAMUI_BUILD_TESTS)
    # idle_budget asserts the application does not repaint while nothing changes.
    foreach(scenario navigation preview search actions idle_budget)
        qamui_add_scenario_test(
            NAME file_manager_${scenario}
            TARGET qamui_file_manager
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_${scenario}.js"
            COLUMNS 100
            ROWS 30
            LABELS AgentSmoke
        )
    endforeach()
endif()
