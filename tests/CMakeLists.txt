if(QMLTUI_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

# One directory per library, mirroring src/QmlTui: Core, Controls, Image,
# Runner and PtyView each register their own tests. The installed
# consumer below is a separate CMake project built against an install tree.

# A Qt Test executable. Every test links the core libraries so a Core test can
# instantiate controls without ceremony; Image is linked when enabled.
function(add_qmltui_test test_name test_source)
    add_executable(${test_name} ${test_source})
    set(qmltui_test_libraries Qt6::Core Qt6::Gui Qt6::Test qmltui::core qmltui::controls)
    if(QMLTUI_ENABLE_IMAGE)
        list(APPEND qmltui_test_libraries qmltui::image)
    endif()
    target_link_libraries(${test_name} PRIVATE ${qmltui_test_libraries})
    target_compile_definitions(${test_name} PRIVATE QMLTUI_SOURCE_DIR="${qmltui_SOURCE_DIR}")
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QML2_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY};QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}"
        LABELS Unit
    )
endfunction()

add_subdirectory(Core)

if(QMLTUI_BUILD_TESTS)
    add_subdirectory(Controls)
    if(QMLTUI_ENABLE_IMAGE)
        add_subdirectory(Image)
    endif()
    add_subdirectory(Runner)
    if(QMLTUI_ENABLE_PTYVIEW)
        add_subdirectory(PtyView)
    endif()

    # Installs this build into a private prefix, then configures, builds and runs
    # tests/installed_consumer as an independent project against it: find_package,
    # every installed header, and a custom item painting through the public API.
    add_test(
        NAME test_installed_custom_item_consumer
        COMMAND "${CMAKE_COMMAND}"
            "-DQMLTUI_BUILD_DIR=${CMAKE_BINARY_DIR}"
            "-DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer"
            "-DCONSUMER_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/installed_consumer_build"
            "-DINSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/installed_consumer_prefix"
            "-DQt6_DIR=${Qt6_DIR}"
            "-DCONSUMER_GENERATOR=${CMAKE_GENERATOR}"
            "-DCONSUMER_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
            "-DCONSUMER_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
            "-DCONSUMER_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer/RunConsumer.cmake"
    )
    set_tests_properties(test_installed_custom_item_consumer PROPERTIES
        LABELS Unit
        TIMEOUT 120
    )

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME test_check_skills
            COMMAND "${Python3_EXECUTABLE}"
                "${qmltui_SOURCE_DIR}/.agents/skills/qmltui-qml/check-skills.py"
                --build-dir "${CMAKE_BINARY_DIR}"
        )
        set_tests_properties(test_check_skills PROPERTIES LABELS Unit)

        add_test(
            NAME test_check_docs
            COMMAND "${Python3_EXECUTABLE}"
                "${qmltui_SOURCE_DIR}/scripts/check_docs.py"
                --build-dir "${CMAKE_BINARY_DIR}"
        )
        set_tests_properties(test_check_docs PROPERTIES LABELS Unit)
    endif()
endif()
