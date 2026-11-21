# A QmlTui application without QML. QmlTui::Core links Qt6::Qml for its type
# registrations. A JavaScript engine is created only for requested Runner sessions.
qt_add_executable(qmltui_cpp_only_demo
    main.cpp
    ServiceDashboard.h
    ServiceDashboard.cpp
)

target_link_libraries(qmltui_cpp_only_demo PRIVATE
    Qt6::Core
    QmlTui::Core
)

if(COMMAND qmltui_wasm_app)
    qmltui_wasm_app(qmltui_cpp_only_demo)
endif()

# A C++-created Terminal needs an explicit environment-driven Runner hook.
if(BUILD_TESTING AND TARGET QmlTui::Runner)
    target_link_libraries(qmltui_cpp_only_demo PRIVATE QmlTui::Runner)
    target_compile_definitions(qmltui_cpp_only_demo PRIVATE QMLTUI_CPP_ONLY_RUNNER)
    qmltui_add_scenario(
        NAME cpp_only_dashboard
        TARGET qmltui_cpp_only_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_cpp_only.js"
    )
endif()
