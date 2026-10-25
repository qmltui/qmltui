cmake_minimum_required(VERSION 3.21)
project(qmltuiInstalledCustomItemConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deliberately without Gui: QmlTuiConfig.cmake has to pull in every Qt module
# the exported targets reference.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
find_package(QmlTui 0.5 REQUIRED)

if(NOT DEFINED QMLTUI_PUBLIC_HEADERS)
    message(FATAL_ERROR "QMLTUI_PUBLIC_HEADERS (the generated list of installed headers) was not provided")
endif()

qt_add_executable(qmltui_installed_custom_item_consumer main.cpp)
# Every installed header is compiled through QMLTUI_PUBLIC_HEADERS, so a header
# that leaks a private include fails here, not on a user's machine.
target_compile_definitions(qmltui_installed_custom_item_consumer PRIVATE
    QMLTUI_PUBLIC_HEADERS="${QMLTUI_PUBLIC_HEADERS}"
    QMLTUI_QML_IMPORT_PATH="${QmlTui_QML_IMPORT_PATH}"
)
target_link_libraries(qmltui_installed_custom_item_consumer PRIVATE
    Qt6::Core
    Qt6::Qml
    qmltui::core
    qmltui::controls
)
if(TARGET qmltui::image)
    target_link_libraries(qmltui_installed_custom_item_consumer PRIVATE qmltui::image)
endif()
