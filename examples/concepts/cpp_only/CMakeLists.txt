# A Qtui application without QML. Qtui::Core links Qt6::Qml for its type
# registrations, but this executable never creates a QML or JavaScript engine.
qt_add_executable(qtui_cpp_only
    main.cpp
    ServiceDashboard.h
    ServiceDashboard.cpp
)

target_link_libraries(qtui_cpp_only PRIVATE
    Qt6::Core
    Qtui::Core
)

# The scenario executable constructs the Runner itself because the auto-hook
# only fires for a Terminal created by a QML engine. Same application code,
# telemetry driven by the script.
if(TARGET Qtui::Runner AND QTUI_BUILD_TESTS)
    qt_add_executable(qtui_cpp_only_test
        main_test.cpp
        ServiceDashboard.h
        ServiceDashboard.cpp
    )
    target_compile_definitions(qtui_cpp_only_test PRIVATE
        QTUI_CPP_ONLY_SCENARIO="${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_only.js"
    )
    target_link_libraries(qtui_cpp_only_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::Runner
    )
    qtui_add_scenario_test(
        NAME cpp_only_dashboard
        TARGET qtui_cpp_only_test
        EMBEDDED
        LABELS AgentSmoke
    )
endif()
