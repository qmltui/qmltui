# Scenario runs against the example applications, driven by the test scripts
# kept next to each app under examples/apps/. Guarded on the target so a tree
# without that example (Behavior off, non-Unix) skips it.

if(TARGET qtui_pos_cashier)
    add_qtui_runner_scenario(test_runner_pos qtui_pos_cashier
        "${Qtui_SOURCE_DIR}/examples/apps/pos_cashier/test_pos.js" 0
        ARTIFACT "pos/receipt.txt"
        ARTIFACT_CONTENT "TRANSACTION COMPLETED"
    )
endif()
if(TARGET qtui_snake_game)
    # The game ticks on QML Timers, so the whole run is virtual time.
    add_qtui_runner_scenario(test_runner_snake qtui_snake_game
        "${Qtui_SOURCE_DIR}/examples/apps/snake_game/test_snake.js" 0
        CLOCK VIRTUAL
        ARTIFACT "snake/paused.txt"
        ARTIFACT_CONTENT "PAUSED"
    )
    add_qtui_runner_scenario(test_runner_snake_ai qtui_snake_game
        "${Qtui_SOURCE_DIR}/examples/apps/snake_game/test_snake_ai.js" 0
        FULL
        CLOCK VIRTUAL
        SCENARIO_TIMEOUT_MS 15000
        ARTIFACT "snake/ai_gameplay.txt"
        ARTIFACT_CONTENT "SCORE"
    )
endif()
if(TARGET qtui_container_fleet)
    add_qtui_runner_scenario(test_runner_container_fleet qtui_container_fleet
        "${Qtui_SOURCE_DIR}/examples/apps/container_fleet/test_fleet.js" 0
        ARTIFACT "fleet/critical.txt"
        ARTIFACT_CONTENT "CRITICAL"
    )
endif()
if(TARGET qtui_pacman_game)
    add_qtui_runner_scenario(test_runner_pacman qtui_pacman_game
        "${Qtui_SOURCE_DIR}/examples/apps/pacman_game/test_pacman.js" 0
        ARTIFACT "pacman/paused.txt"
        ARTIFACT_CONTENT "PAUSED"
    )
endif()
if(TARGET qtui_sql_studio)
    add_qtui_runner_scenario(test_runner_sql_studio qtui_sql_studio
        "${Qtui_SOURCE_DIR}/examples/apps/sql_studio/test_sql_studio.js" 0
        ARTIFACT "sql_studio_verified.txt"
        ARTIFACT_CONTENT "SQLStudio TUI"
    )
endif()
if(TARGET qtui_industrial_plc_hmi)
    # Stays on the real clock: the scenario waits on simulation state with
    # waitForCondition(), which does not advance virtual time.
    add_qtui_runner_scenario(test_runner_plc qtui_industrial_plc_hmi
        "${Qtui_SOURCE_DIR}/examples/apps/industrial_plc_hmi/test_plc.js" 0
        FULL
        PROCESS_TIMEOUT 35
        OPERATION_TIMEOUT_MS 5000
        SCENARIO_TIMEOUT_MS 30000
        ARTIFACT "plc/reset.txt"
        ARTIFACT_CONTENT "STATE: IDLE (ALARM RESET)"
    )
endif()
if(TARGET qtui_twozero48)
    add_qtui_runner_scenario(test_runner_twozero48 qtui_twozero48
        "${Qtui_SOURCE_DIR}/examples/apps/twozero48/test_twozero48.js" 0
        ARTIFACT "twozero48/initial.txt"
        ARTIFACT_CONTENT "2048 TUI"
    )
endif()
