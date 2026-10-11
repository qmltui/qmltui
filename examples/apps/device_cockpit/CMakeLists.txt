find_package(Qt6 REQUIRED COMPONENTS Network DBus)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_executable(qamui_device_cockpit main.cpp)

qt_add_qml_module(qamui_device_cockpit
    URI Qamui.Examples.DeviceCockpit
    VERSION 1.0
    SOURCES
        CockpitBackend.h
        CockpitBackend.cpp
        SystemInfoProvider.h
        SystemInfoProvider.cpp
        MockSystemInfoProvider.h
        MockSystemInfoProvider.cpp
        LinuxSystemInfoProvider.h
        LinuxSystemInfoProvider.cpp
        ServiceProvider.h
        ServiceProvider.cpp
        MockServiceProvider.h
        MockServiceProvider.cpp
        SystemdServiceProvider.h
        SystemdServiceProvider.cpp
        LogProvider.h
        LogProvider.cpp
        MockLogProvider.h
        MockLogProvider.cpp
        JournaldLogProvider.h
        JournaldLogProvider.cpp
        NetworkProvider.h
        NetworkProvider.cpp
        MockNetworkProvider.h
        MockNetworkProvider.cpp
        LinuxNetworkProvider.h
        LinuxNetworkProvider.cpp
    QML_FILES
        StatusBadge.qml
        Main.qml
        Theme.qml
        HeaderPanel.qml
        OverviewPage.qml
        NetworkPage.qml
        ServicesPage.qml
        LogsPage.qml
        ConfirmDialog.qml
        EditIpv4Dialog.qml
    DEPENDENCIES
        TARGET Qamui::Core
        TARGET Qamui::Controls
        TARGET Qamui::Controls
)

target_link_libraries(qamui_device_cockpit PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Network
    Qt6::DBus
    Qamui::Core
    Qamui::Controls
    Qamui::Controls
)

# Carries the scenario runner: the application executes QAMUI_RUNNER_SCRIPT/QAMUI_RUNNER_EVAL
# scenarios (against the mock providers by default, see main.cpp) when those
# variables are set in its environment.
if(TARGET Qamui::Runner)
    target_link_libraries(qamui_device_cockpit PRIVATE Qamui::Runner)
endif()

if(QAMUI_INSTALL_EXAMPLES)
    install(TARGETS qamui_device_cockpit RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if(TARGET Qamui::Runner AND QAMUI_BUILD_TESTS)
    # Module scenarios (.mjs): shared navigation lives in lib/cockpit.mjs.
    foreach(scenario overview network services logs resize)
        qamui_add_scenario_test(
            NAME device_cockpit_${scenario}
            TARGET qamui_device_cockpit
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_${scenario}.mjs"
            COLUMNS 100
            ROWS 30
            LABELS AgentSmoke
        )
    endforeach()
endif()
