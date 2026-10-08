# Engine Include Hygiene Check
# Ensures src/Qtui/Core/engine/ does not include any item headers other than Item.h and Terminal.h.

file(GLOB_RECURSE ENGINE_FILES "${Qtui_SOURCE_DIR}/src/Qtui/Core/engine/*.*")

list(LENGTH ENGINE_FILES ENGINE_FILES_COUNT)
if(ENGINE_FILES_COUNT EQUAL 0)
    message(FATAL_ERROR "CheckEngineIncludes: No engine files found to check in ${Qtui_SOURCE_DIR}/src/Qtui/Core/engine/")
endif()

foreach(FILE_PATH ${ENGINE_FILES})
    file(STRINGS "${FILE_PATH}" INCLUDE_LINES REGEX "^[ \t]*#[ \t]*include[ \t]+[\"<].*items/.*[\">]")
    foreach(LINE ${INCLUDE_LINES})
        if(NOT LINE MATCHES "items/(Item\\.h|Terminal\\.h)")
            message(FATAL_ERROR "Include hygiene violation in ${FILE_PATH}: '${LINE}'. "
                                "Engine layer must not depend directly on concrete item controls.")
        endif()
    endforeach()
endforeach()
