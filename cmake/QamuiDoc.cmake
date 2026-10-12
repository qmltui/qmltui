# Generation of Qt Compressed Help (.qch) archive for Qt Creator F1 integration.

find_program(QAMUI_QHELPGENERATOR qhelpgenerator
    HINTS "/usr/lib/qt6"
          "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}"
          "${QT6_INSTALL_PREFIX}/bin")
find_package(Python3 COMPONENTS Interpreter)

if(QAMUI_QHELPGENERATOR AND Python3_Interpreter_FOUND)
    set(qamui_doc_dir "${CMAKE_BINARY_DIR}/doc")
    add_custom_target(docs_qch
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/build_qch.py"
                --source "${CMAKE_SOURCE_DIR}" --out "${qamui_doc_dir}"
        COMMAND ${QAMUI_QHELPGENERATOR} "${qamui_doc_dir}/qamui.qhp" -o "${qamui_doc_dir}/qamui.qch"
        COMMENT "Generating qamui.qch for Qt Creator"
    )
    install(FILES "${qamui_doc_dir}/qamui.qch"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT Documentation OPTIONAL)
endif()
