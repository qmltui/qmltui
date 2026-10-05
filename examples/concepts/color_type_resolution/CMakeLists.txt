qt_add_executable(qtui_color_type_resolution
    main.cpp
)

target_link_libraries(qtui_color_type_resolution PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
)

target_compile_definitions(qtui_color_type_resolution PRIVATE
    QTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)

if(QTUI_BUILD_TESTS)
    add_test(
        NAME color_type_resolution
        COMMAND qtui_color_type_resolution
    )
    set_tests_properties(color_type_resolution PROPERTIES
        LABELS AgentSmoke
    )
endif()
