# Generation of Qt Compressed Help (.qch) archive for Qt Creator F1 integration.

find_program(QMLTUI_QHELPGENERATOR qhelpgenerator
    HINTS "/usr/lib/qt6"
          "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBEXECS}"
          "${QT6_INSTALL_PREFIX}/bin")
find_package(Python3 COMPONENTS Interpreter)

if(QMLTUI_QHELPGENERATOR AND Python3_Interpreter_FOUND)
    set(qmltui_doc_dir "${CMAKE_BINARY_DIR}/doc")
    add_custom_target(docs_qch
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/build_qch.py"
                --source "${CMAKE_SOURCE_DIR}" --out "${qmltui_doc_dir}"
        COMMAND ${QMLTUI_QHELPGENERATOR} "${qmltui_doc_dir}/qmltui.qhp" -o "${qmltui_doc_dir}/qmltui.qch"
        COMMENT "Generating qmltui.qch for Qt Creator"
    )
    install(FILES "${qmltui_doc_dir}/qmltui.qch"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}" COMPONENT Documentation OPTIONAL)
endif()
