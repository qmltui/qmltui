cmake_minimum_required(VERSION 3.21)
project(QamuiInstalledCustomItemConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Deliberately without Gui: QamuiConfig.cmake has to pull in every Qt module
# the exported targets reference.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Qml)
find_package(Qamui 0.5 REQUIRED)

if(NOT DEFINED QAMUI_PUBLIC_HEADERS)
    message(FATAL_ERROR "QAMUI_PUBLIC_HEADERS (the generated list of installed headers) was not provided")
endif()

qt_add_executable(qamui_installed_custom_item_consumer main.cpp)
# Every installed header is compiled through QAMUI_PUBLIC_HEADERS, so a header
# that leaks a private include fails here, not on a user's machine.
target_compile_definitions(qamui_installed_custom_item_consumer PRIVATE
    QAMUI_PUBLIC_HEADERS="${QAMUI_PUBLIC_HEADERS}"
)
target_link_libraries(qamui_installed_custom_item_consumer PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
    Qamui::Controls
)
if(TARGET Qamui::Image)
    target_link_libraries(qamui_installed_custom_item_consumer PRIVATE Qamui::Image)
endif()
