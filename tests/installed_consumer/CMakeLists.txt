cmake_minimum_required(VERSION 3.21)
project(QtuiInstalledCustomItemConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deliberately without Gui: QtuiConfig.cmake has to pull in every Qt module
# the exported targets reference.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
find_package(Qtui 0.4 REQUIRED)

if(NOT DEFINED QTUI_PUBLIC_HEADERS)
    message(FATAL_ERROR "QTUI_PUBLIC_HEADERS (the generated list of installed headers) was not provided")
endif()

qt_add_executable(qtui_installed_custom_item_consumer main.cpp)
# Every installed header is compiled through QTUI_PUBLIC_HEADERS, so a header
# that leaks a private include fails here, not on a user's machine.
target_compile_definitions(qtui_installed_custom_item_consumer PRIVATE
    QTUI_PUBLIC_HEADERS="${QTUI_PUBLIC_HEADERS}"
)
target_link_libraries(qtui_installed_custom_item_consumer PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
    Qtui::Image
)
