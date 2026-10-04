qt_add_executable(qtui_container_fleet main.cpp FleetListModel.cpp FleetListModel.h)

qt_add_qml_module(qtui_container_fleet
    URI ContainerFleetApp
    VERSION 1.0
    SOURCES
        FleetListModel.cpp
        FleetListModel.h
    QML_FILES
        Main.qml

    DEPENDENCIES TARGET Qtui::Core Qtui::Controls Qtui::Extras
)

target_link_libraries(qtui_container_fleet PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::Extras)

if(TARGET Qtui::Testing)
    target_link_libraries(qtui_container_fleet PRIVATE Qtui::Testing)
endif()
