set(btop_sources
    BtopModel.cpp
    BtopModel.h
)

qt_add_executable(qamui_btop
    main.cpp
    ${btop_sources}
)

qt_add_qml_module(qamui_btop
    URI BtopDemo
    VERSION 1.0
    QML_FILES Main.qml
    DEPENDENCIES
 TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_btop PRIVATE
    Qt6::Core
    Qt6::Qml
    Qamui::Core
    Qamui::Core
    Qamui::Controls
    Qamui::Controls
)

if(TARGET Qamui::Runner)

    qt_add_executable(qamui_btop_test
        main.cpp
        ${btop_sources}
    )

    qt_add_qml_module(qamui_btop_test
        URI BtopDemo
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qml_test/BtopDemo"
        VERSION 1.0
        QML_FILES Main.qml
        DEPENDENCIES
        TARGET Qamui::Core Qamui::Controls
    )

    target_link_libraries(qamui_btop_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qamui::Core
        Qamui::Core
        Qamui::Controls
        Qamui::Controls
        Qamui::Runner
    )

    qamui_add_scenario_test(
        NAME btop_smoke
        TARGET qamui_btop_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_btop.js"
        COLUMNS 132
        ROWS 42
        LABELS AgentSmoke
    )

    # Only timer-driven refreshes may repaint while the user is idle.
    qamui_add_scenario_test(
        NAME btop_idle_budget
        TARGET qamui_btop_test
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_idle_budget.js"
        COLUMNS 132
        ROWS 42
        LABELS AgentSmoke
    )
endif()
