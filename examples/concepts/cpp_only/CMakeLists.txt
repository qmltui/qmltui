qt_add_executable(qtui_cpp_only main.cpp)

target_link_libraries(qtui_cpp_only PRIVATE
    Qt6::Core
    Qtui::Core
)
