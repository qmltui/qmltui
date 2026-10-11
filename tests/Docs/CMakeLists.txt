# Documentation examples are compiled, loaded and rendered like any other
# scenario: scripts/check_doc_examples.py extracts every ```qml block under
# docs/qml-types into a Main.qml and runs it through this runner in capture
# mode. A block that does not load, has a non-Terminal root, or fails to
# render a frame fails test_component_doc_examples.

add_executable(qamui_doc_example_runner main.cpp)
set(qamui_doc_example_runner_libs Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Runner)
if(QAMUI_ENABLE_IMAGE)
    list(APPEND qamui_doc_example_runner_libs Qamui::Image)
endif()
target_link_libraries(qamui_doc_example_runner PRIVATE ${qamui_doc_example_runner_libs})

if(Python3_Interpreter_FOUND)
    add_test(NAME test_component_doc_examples
        COMMAND "${Python3_EXECUTABLE}" "${Qamui_SOURCE_DIR}/scripts/check_doc_examples.py"
                --runner "$<TARGET_FILE:qamui_doc_example_runner>"
                --qml-import-path "${QT_QML_OUTPUT_DIRECTORY}"
                --artifact-dir "${CMAKE_CURRENT_BINARY_DIR}/DocExampleArtifacts")
    set_tests_properties(test_component_doc_examples PROPERTIES LABELS AgentSmoke TIMEOUT 300)
endif()

if(Python3_Interpreter_FOUND)
    add_test(NAME test_doc_includes
        COMMAND "${Python3_EXECUTABLE}" "${Qamui_SOURCE_DIR}/scripts/check_doc_includes.py")
    set_tests_properties(test_doc_includes PROPERTIES LABELS "Tutorial;AgentSmoke")
endif()
