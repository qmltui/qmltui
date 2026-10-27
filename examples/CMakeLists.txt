set(QMLTUI_RECORD_DIR "${PROJECT_SOURCE_DIR}/docs/assets/casts")
find_program(QMLTUI_AGG_EXECUTABLE NAMES agg)
if(QMLTUI_AGG_EXECUTABLE AND NOT DEFINED QMLTUI_RECORD_POST_COMMAND)
    set(QMLTUI_RECORD_POST_COMMAND
        "${QMLTUI_AGG_EXECUTABLE}"
        "--renderer"
        "resvg"
        "--theme"
        "0d1117,c9d1d9,0d1117,ff7b72,2cde85,f9c74f,58a6ff,d2a8ff,39c5cf,c9d1d9,484f58,ffa198,00ff66,fbbf24,79c0ff,e2c5ff,56d4dd,f0f6fc"
        "--text-font-family"
        "JetBrainsMono Nerd Font,JetBrains Mono,Fira Code,SF Mono,Menlo,Consolas,DejaVu Sans Mono,Liberation Mono"
        "--font-size"
        "24"
        "--line-height"
        "1.08"
        "<CAST>"
        "<CAST_STEM>.gif"
    )
endif()

add_subdirectory(helloworld)
add_subdirectory(recipes)
add_subdirectory(gallery)
