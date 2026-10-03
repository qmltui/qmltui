

add_subdirectory(Core)
add_subdirectory(Canvas)
add_subdirectory(Controls)
add_subdirectory(Extras)
if(QTUI_BUILD_TERMINAL_HOST)
    add_subdirectory(TerminalHost)
endif()
