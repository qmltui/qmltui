find_package(Qt6 REQUIRED COMPONENTS TaskTree)

qt_add_qml_module(qtasktree_model
    STATIC
    URI QTaskTree.App
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/QTaskTree/App"
    SOURCES
        TaskTreeModel.h
        TaskTreeModel.cpp
)

target_include_directories(qtasktree_model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qtasktree_model PUBLIC Qt6::Core Qt6::Qml Qt6::TaskTree)

qt_add_executable(qtasktree_runner
    main.cpp
)

qt_add_qml_module(qtasktree_runner
    URI "qtasktree_runner"
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET Qtui TerminalWidgets TerminalControls
)

target_link_libraries(qtasktree_runner PRIVATE
    Qt6::Core
    Qt6::Qml
    qtasktree_model
    qtasktree_modelplugin
    Qtui
    TerminalWidgets
    TerminalControls
)

set_target_properties(qtasktree_runner PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
