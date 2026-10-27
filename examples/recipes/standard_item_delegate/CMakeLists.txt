# QStandardItemModel lives in Qt6::Gui; the example links it for the model
# only and still runs under a plain QCoreApplication.
find_package(Qt6 REQUIRED COMPONENTS Gui)

# The QML module is owned by each executable (not a shared static library) so
# Qt records the build tree's QML import path for it and `import QmlTui.Controls`
# resolves when the binary is launched from Qt Creator or a shell.
qt_add_executable(qmltui_standard_item_delegate_demo main.cpp)
qt_add_qml_module(qmltui_standard_item_delegate_demo
    URI StandardItemDelegateDemo
    VERSION 1.0
    QML_FILES
        Main.qml
        StandardItemDelegate.qml
    SOURCES ReleaseChecklistModel.h ReleaseChecklistModel.cpp
    DEPENDENCIES TARGET qmltui::core
)
target_link_libraries(qmltui_standard_item_delegate_demo PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml qmltui::core)

if(BUILD_TESTING AND TARGET qmltui::runner)
    target_link_libraries(qmltui_standard_item_delegate_demo PRIVATE qmltui::runner)
    qmltui_add_scenario(
        NAME standard_item_delegate_smoke
        TARGET qmltui_standard_item_delegate_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_standard_item_delegate.js"
        COLUMNS 80
        ROWS 24
    )
endif()

# Optional Qt Widgets reference viewer for the same model, so the terminal
# rendering can be compared with QStyledItemDelegate side by side.
option(QMLTUI_BUILD_WIDGETS_REFERENCE
    "Build the Qt Widgets reference viewer of the standard_item_delegate example" ON)
if(QMLTUI_BUILD_WIDGETS_REFERENCE)
    find_package(Qt6 OPTIONAL_COMPONENTS Widgets QUIET)
endif()
if(QMLTUI_BUILD_WIDGETS_REFERENCE AND TARGET Qt6::Widgets)
    qt_add_executable(qmltui_standard_item_delegate_widgets
        widgets_main.cpp ReleaseChecklistModel.h ReleaseChecklistModel.cpp)
    # Qt6::Qml only for the QML_ELEMENT declaration in the shared model header.
    target_link_libraries(qmltui_standard_item_delegate_widgets PRIVATE
        Qt6::Core Qt6::Gui Qt6::Qml Qt6::Widgets)
    if(QMLTUI_BUILD_TESTS)
        add_test(NAME standard_item_delegate_widgets_smoke
            COMMAND qmltui_standard_item_delegate_widgets --smoke)
        set_tests_properties(standard_item_delegate_widgets_smoke PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen" LABELS Unit)
    endif()
    message(STATUS "standard_item_delegate: Qt Widgets found, building the reference viewer")
else()
    message(STATUS "standard_item_delegate: Qt Widgets reference viewer skipped")
endif()
