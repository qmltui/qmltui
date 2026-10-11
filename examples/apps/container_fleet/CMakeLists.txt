qt_add_executable(qamui_container_fleet main.cpp FleetListModel.cpp FleetListModel.h)

qt_add_qml_module(qamui_container_fleet
    URI ContainerFleetApp
    VERSION 1.0
    SOURCES
        FleetListModel.cpp
        FleetListModel.h
    QML_FILES
        Main.qml

 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_container_fleet PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_container_fleet PRIVATE Qamui::Runner)
endif()

if(QAMUI_INSTALL_EXAMPLES)
    install(TARGETS qamui_container_fleet RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
