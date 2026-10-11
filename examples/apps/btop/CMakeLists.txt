set(btop_sources
    BtopModel.cpp
    BtopModel.h
)

qt_add_executable(qtui_btop
    main.cpp
    ${btop_sources}
)

qt_add_qml_module(qtui_btop
    URI BtopDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES
 TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_btop PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Core
    Qtui::Controls
    Qtui::Controls
)

if(TARGET Qtui::Runner)

    qt_add_executable(qtui_btop_test
        main.cpp
        ${btop_sources}
    )

    qt_add_qml_module(qtui_btop_test
        URI BtopDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/BtopDemo"
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES
        TARGET Qtui::Core Qtui::Controls
    )

    target_link_libraries(qtui_btop_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::Core
        Qtui::Controls
        Qtui::Controls
        Qtui::Runner
    )

    qtui_add_scenario_test(
        NAME btop_smoke
        TARGET qtui_btop_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_btop.js"
        COLUMNS 132
        ROWS 42
        LABELS AgentSmoke
    )

    # Only timer-driven refreshes may repaint while the user is idle.
    qtui_add_scenario_test(
        NAME btop_idle_budget
        TARGET qtui_btop_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 132
        ROWS 42
        LABELS AgentSmoke
        LIVE
    )
endif()
