qt_add_executable(qmltui_color_type_resolution_demo
    main.cpp
)

target_link_libraries(qmltui_color_type_resolution_demo PRIVATE
    Qt6::Core
    Qt6::Qml
    qmltui::core
)

target_compile_definitions(qmltui_color_type_resolution_demo PRIVATE
    QMLTUI_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
)

if(QMLTUI_BUILD_TESTS)
    add_test(
        NAME color_type_resolution
        COMMAND qmltui_color_type_resolution_demo
    )
    set_tests_properties(color_type_resolution PROPERTIES
        LABELS AgentSmoke
    )
endif()
