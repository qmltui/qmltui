qt_add_executable(qmltui-runner main.cpp)
target_link_libraries(qmltui-runner PRIVATE Qt6::Core)
target_compile_definitions(qmltui-runner PRIVATE QMLTUI_VERSION="${PROJECT_VERSION}")
add_executable(QmlTui::qmltui-runner ALIAS qmltui-runner)
set_target_properties(qmltui-runner PROPERTIES EXPORT_NAME qmltui-runner)
qmltui_set_install_rpath(qmltui-runner "${CMAKE_INSTALL_BINDIR}")
if(QMLTUI_ENABLE_RUNNER)
    install(TARGETS qmltui-runner
        EXPORT QmlTuiRunnerTargets
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()
