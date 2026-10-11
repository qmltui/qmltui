# A Qamui application without QML. Qamui::Core links Qt6::Qml for its type
# registrations. A JavaScript engine is created only for requested Runner sessions.
qt_add_executable(qamui_cpp_only_demo
    main.cpp
    ServiceDashboard.h
    ServiceDashboard.cpp
)

target_link_libraries(qamui_cpp_only_demo PRIVATE
    Qt6::Core
    Qamui::Core
)

# A C++-created Terminal needs an explicit environment-driven Runner hook.
if(TARGET Qamui::Runner)
    target_link_libraries(qamui_cpp_only_demo PRIVATE Qamui::Runner)
    target_compile_definitions(qamui_cpp_only_demo PRIVATE QAMUI_CPP_ONLY_RUNNER)
    qamui_add_scenario(
        NAME cpp_only_dashboard
        TARGET qamui_cpp_only_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_only.js"
        LABELS AgentSmoke
    )
endif()
