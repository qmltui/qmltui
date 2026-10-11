qt_add_executable(qamui_color_type_resolution
    main.cpp
)

target_link_libraries(qamui_color_type_resolution PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
)

target_compile_definitions(qamui_color_type_resolution PRIVATE
    QAMUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)

if(QAMUI_BUILD_TESTS)
    add_test(
        NAME color_type_resolution
        COMMAND qamui_color_type_resolution
    )
    set_tests_properties(color_type_resolution PROPERTIES
        LABELS AgentSmoke
    )
endif()
