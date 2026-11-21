cmake_minimum_required(VERSION 3.21)
project(qmltuiInstalledCustomItemConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deliberately without Gui: QmlTuiConfig.cmake has to pull in every Qt module
# the exported targets reference.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
qt_standard_project_setup(REQUIRES 6.8)
find_package(QmlTui 0.6 REQUIRED COMPONENTS Core Controls)
if(TARGET qmltui::core OR TARGET qmltui::controls)
    message(FATAL_ERROR "Legacy lowercase qmltui:: aliases must not be exported")
endif()

if(NOT DEFINED QMLTUI_PUBLIC_HEADERS)
    message(FATAL_ERROR "QMLTUI_PUBLIC_HEADERS (the generated list of installed headers) was not provided")
endif()

qt_add_executable(qmltui_installed_custom_item_consumer main.cpp)
qt_add_qml_module(qmltui_installed_custom_item_consumer
    URI InstalledConsumer
    QML_FILES ConsumerMain.qml
)
# Every installed header is compiled through QMLTUI_PUBLIC_HEADERS, so a header
# that leaks a private include fails here, not on a user's machine.
target_compile_definitions(qmltui_installed_custom_item_consumer PRIVATE
    QMLTUI_PUBLIC_HEADERS="${QMLTUI_PUBLIC_HEADERS}"
    QMLTUI_QML_IMPORT_PATH="${QmlTui_QML_IMPORT_PATH}"
)
target_link_libraries(qmltui_installed_custom_item_consumer PRIVATE
    Qt6::Core
    Qt6::Qml
    QmlTui::Core
    QmlTui::Controls
)
if(TARGET QmlTui::Image)
    target_link_libraries(qmltui_installed_custom_item_consumer PRIVATE QmlTui::Image)
endif()
