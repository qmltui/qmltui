add_library(QtuiTesting
    TestRunner.h
    TestRunner.cpp
)

add_library(Qtui::Testing ALIAS QtuiTesting)
set_target_properties(QtuiTesting PROPERTIES EXPORT_NAME Testing)

target_include_directories(QtuiTesting PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qtui>
)

target_link_libraries(QtuiTesting
    PUBLIC
        Qt6::Qml
        Qtui::Core
    PRIVATE
        Qt6::Gui
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(QtuiTesting INTERFACE
        $<$<LINK_LANGUAGE:CXX>:SHELL:-Wl,--whole-archive $<TARGET_FILE:QtuiTesting> -Wl,--no-whole-archive>
    )
endif()

if(QTUI_ENABLE_TEST_RUNNER OR QTUI_ENABLE_TEST_DRIVER)
    install(TARGETS QtuiTesting
        EXPORT QtuiTargets
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    )
    install(FILES TestRunner.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/Testing"
    )
    install(FILES ../items/Terminal.h ../items/Rectangle.h
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/qtui/items"
    )
endif()
