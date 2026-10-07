# Qt Multimedia is optional: with it the example plays files and analyzes the
# microphone; without it only the procedural demo source is available.
find_package(Qt6 OPTIONAL_COMPONENTS Multimedia QUIET)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Each executable owns the QML module (as table_view does) so Qt writes the
# build tree's QML import path into a qt.conf beside it and `import Qtui.Controls`
# resolves when the binary is launched from Qt Creator or a shell. A shared
# static module library would not get that treatment, and since no C++ symbol
# of Qtui::Controls is referenced the linker drops the library that carries
# the module's qmldir.
set(cyber_audio_sources
    AudioEngine.h
    AudioEngine.cpp
    SpectrumAnalyzer.h
    SpectrumAnalyzer.cpp
    DemoSynth.h
    DemoSynth.cpp
    Playlist.h
    Playlist.cpp
    WaveformTrace.h
    WaveformTrace.cpp
    ScopeArt.h
    ScopeArt.cpp
)
set(cyber_audio_qml_files
    Main.qml
    Theme.qml
    HeaderBar.qml
    TransportBar.qml
    KeyMapBar.qml
    KeyHint.qml
    SpectrumPanel.qml
    ScopePanel.qml
    VuMeter.qml
)

# Adds an executable that compiles the module under its own URI.
function(cyber_audio_add_executable target uri main_source)
    qt_add_executable(${target} ${main_source})
    qt_add_qml_module(${target}
        URI ${uri}
        VERSION 1.0
        RESOURCE_PREFIX "/qt/qml"
        SOURCES ${cyber_audio_sources}
        QML_FILES ${cyber_audio_qml_files}
        DEPENDENCIES
            TARGET Qtui::Core
            TARGET Qtui::Controls
    )
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls)
    if(TARGET Qt6::Multimedia)
        target_link_libraries(${target} PRIVATE Qt6::Multimedia)
        target_compile_definitions(${target} PRIVATE CYBER_AUDIO_HAVE_MULTIMEDIA=1)
    else()
        target_compile_definitions(${target} PRIVATE CYBER_AUDIO_HAVE_MULTIMEDIA=0)
    endif()
endfunction()

if(TARGET Qt6::Multimedia)
    message(STATUS "cyber_audio: Qt Multimedia found, file playback and microphone enabled")
else()
    message(STATUS "cyber_audio: Qt Multimedia not found, demo source only")
endif()

cyber_audio_add_executable(qtui_cyber_audio CyberAudio main.cpp)

if(TARGET Qtui::TestRunner)
    cyber_audio_add_executable(qtui_cyber_audio_test CyberAudioTest main_test.cpp)
    target_link_libraries(qtui_cyber_audio_test PRIVATE Qtui::TestRunner)

    if(QTUI_BUILD_TESTS)
        include("${PROJECT_SOURCE_DIR}/cmake/QtuiTestRunner.cmake")
        qtui_add_scenario_test(
            NAME cyber_audio_demo_smoke
            TARGET qtui_cyber_audio_test
            SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_cyber_audio.js"
            COLUMNS 120
            ROWS 36
            ARTIFACT "cyber_audio/initial.txt"
            ARTIFACT_CONTENT "NEURAL AUDIO MATRIX"
            LABELS AgentSmoke
        )
    endif()
endif()

if(QTUI_BUILD_TESTS)
    qt_add_executable(test_cyber_audio_dsp test_cyber_audio_dsp.cpp ${cyber_audio_sources})
    target_include_directories(test_cyber_audio_dsp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(test_cyber_audio_dsp PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qtui::Core
    )
    if(TARGET Qt6::Multimedia)
        target_link_libraries(test_cyber_audio_dsp PRIVATE Qt6::Multimedia)
        target_compile_definitions(test_cyber_audio_dsp PRIVATE CYBER_AUDIO_HAVE_MULTIMEDIA=1)
    else()
        target_compile_definitions(test_cyber_audio_dsp PRIVATE CYBER_AUDIO_HAVE_MULTIMEDIA=0)
    endif()
    add_test(NAME cyber_audio_dsp COMMAND test_cyber_audio_dsp)
    set_tests_properties(cyber_audio_dsp PROPERTIES LABELS AgentSmoke)
endif()
