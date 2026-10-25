set(QMLTUI_RECORD_DIR "${PROJECT_SOURCE_DIR}/docs/assets/casts")
find_program(QMLTUI_AGG_EXECUTABLE NAMES agg)
if(QMLTUI_AGG_EXECUTABLE AND NOT DEFINED QMLTUI_RECORD_POST_COMMAND)
    set(QMLTUI_RECORD_POST_COMMAND "${QMLTUI_AGG_EXECUTABLE}" "<CAST>" "<CAST_STEM>.gif")
endif()

add_subdirectory(helloworld)
add_subdirectory(recipes)
add_subdirectory(gallery)
