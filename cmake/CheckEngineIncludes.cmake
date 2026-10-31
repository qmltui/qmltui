# Engine Include Hygiene Check
# 1. Ensures src/QmlTui/Core/Engine/ does not include any item headers other than Item.h and Terminal.h.
# 2. Ensures Qt private headers (<private/...> or <QtQml/private/...>) are isolated to QmlPrivateBridge*,
#    PropertyChanges_p.h, and Behavior.h.

file(GLOB_RECURSE ENGINE_FILES "${qmltui_SOURCE_DIR}/src/QmlTui/Core/Engine/*.*")

list(LENGTH ENGINE_FILES ENGINE_FILES_COUNT)
if(ENGINE_FILES_COUNT EQUAL 0)
    message(FATAL_ERROR "CheckEngineIncludes: No engine files found to check in ${qmltui_SOURCE_DIR}/src/QmlTui/Core/Engine/")
endif()

foreach(FILE_PATH ${ENGINE_FILES})
    file(STRINGS "${FILE_PATH}" INCLUDE_LINES REGEX "^[ \t]*#[ \t]*include[ \t]+[\"<].*Items/.*[\">]")
    foreach(LINE ${INCLUDE_LINES})
        if(NOT LINE MATCHES "Items/(Item\\.h|Terminal\\.h)")
            message(FATAL_ERROR "Include hygiene violation in ${FILE_PATH}: '${LINE}'. "
                                "Engine layer must not depend directly on concrete item controls.")
        endif()
    endforeach()

    if(NOT FILE_PATH MATCHES "Engine/(QmlPrivateBridge_p\\.h|QmlPrivateBridge\\.cpp|PropertyChanges_p\\.h|Behavior\\.h)$")
        file(STRINGS "${FILE_PATH}" PRIVATE_QT_LINES REGEX "^[ \t]*#[ \t]*include[ \t]+[\"<](Qt[^/]*/)?private/.*[\">]")
        foreach(LINE ${PRIVATE_QT_LINES})
            message(FATAL_ERROR "Qt private include violation in ${FILE_PATH}: '${LINE}'. "
                                "Isolate Qt6::QmlPrivate includes to QmlPrivateBridge_p.h / QmlPrivateBridge.cpp.")
        endforeach()
    endif()
endforeach()
