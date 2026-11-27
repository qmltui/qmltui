if(NOT TARGET Qt6::Network)
    find_package(Qt6 6.8 QUIET OPTIONAL_COMPONENTS Network)
endif()

qt_add_executable(qmltui-runner
    main.cpp
    Formatters.h
    Formatters.cpp
    ScenarioExporter.h
    ScenarioExporter.cpp
    ChildSession.h
    ChildSession.cpp
    SessionCommand.h
    SessionCommand.cpp
    SessionConnect.h
    SessionConnect.cpp
    SessionDaemon.h
    SessionDaemon.cpp
    SessionCli.h
    SessionCli.cpp
    TerminalSize.h
)
if(WIN32)
    target_sources(qmltui-runner PRIVATE TerminalSizeWin32.cpp)
else()
    target_sources(qmltui-runner PRIVATE TerminalSizePosix.cpp)
endif()
target_link_libraries(qmltui-runner PRIVATE Qt6::Core)
if(TARGET Qt6::Network)
    target_link_libraries(qmltui-runner PRIVATE Qt6::Network)
    target_compile_definitions(qmltui-runner PRIVATE QMLTUI_RUNNER_HAS_SESSION_DAEMON=1)
else()
    message(WARNING
        "QmlTui: Qt6::Network was not found; building qmltui-runner without persistent "
        "'session' daemon support (--interactive and batch modes remain available).")
    target_compile_definitions(qmltui-runner PRIVATE QMLTUI_RUNNER_HAS_SESSION_DAEMON=0)
endif()
target_compile_definitions(qmltui-runner PRIVATE QMLTUI_VERSION="${PROJECT_VERSION}")
add_executable(QmlTui::qmltui-runner ALIAS qmltui-runner)
set_target_properties(qmltui-runner PROPERTIES EXPORT_NAME qmltui-runner)
qmltui_set_install_rpath(qmltui-runner "${CMAKE_INSTALL_BINDIR}")
if(QMLTUI_ENABLE_RUNNER)
    install(TARGETS qmltui-runner
        EXPORT QmlTuiRunnerTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT runner-cli
    )
endif()
