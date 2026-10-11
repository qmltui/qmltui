# A Qamui application without QML. Qamui::Core links Qt6::Qml for its type
# registrations, but this executable never creates a QML or JavaScript engine.
qt_add_executable(qamui_cpp_only_demo
    main.cpp
    ServiceDashboard.h
    ServiceDashboard.cpp
)

target_link_libraries(qamui_cpp_only_demo PRIVATE
    Qt6::Core
    Qamui::Core
)

# The scenario executable constructs the Runner itself because the auto-hook
# only fires for a Terminal created by a QML engine. Same application code,
# telemetry driven by the script.
if(TARGET Qamui::Runner AND QAMUI_BUILD_TESTS)
    qt_add_executable(qamui_cpp_only_demo_test
        main_test.cpp
        ServiceDashboard.h
        ServiceDashboard.cpp
    )
    target_compile_definitions(qamui_cpp_only_demo_test PRIVATE
        QAMUI_CPP_ONLY_SCENARIO="${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_only.js"
    )
    target_link_libraries(qamui_cpp_only_demo_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qamui::Core
        Qamui::Runner
    )
    qamui_add_scenario_test(
        NAME cpp_only_dashboard
        TARGET qamui_cpp_only_demo_test
        EMBEDDED
        LABELS AgentSmoke
    )
endif()
