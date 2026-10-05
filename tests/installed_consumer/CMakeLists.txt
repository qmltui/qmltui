cmake_minimum_required(VERSION 3.21)
project(QtuiInstalledCustomItemConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
find_package(Qtui 0.4 REQUIRED)

qt_add_executable(qtui_installed_custom_item_consumer main.cpp)
target_link_libraries(qtui_installed_custom_item_consumer PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
)
