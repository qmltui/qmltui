# Qt Multimedia is optional: with it the example plays files and analyzes the
# microphone; without it only the procedural demo source is available.
find_package(Qt6 OPTIONAL_COMPONENTS Multimedia QUIET)

set_source_files_properties(Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(qtui_cyber_audio_module STATIC)

qt_add_qml_module(qtui_cyber_audio_module
    URI CyberAudio
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    SOURCES
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
    QML_FILES
        Main.qml
        Theme.qml
        HeaderBar.qml
        TransportBar.qml
        KeyMapBar.qml
        KeyHint.qml
        SpectrumPanel.qml
        ScopePanel.qml
        VuMeter.qml
    DEPENDENCIES
        TARGET Qtui::Core
        TARGET Qtui::Controls
)

target_include_directories(qtui_cyber_audio_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(qtui_cyber_audio_module PRIVATE
    Qt6::Core
    Qt6::Qml
    Qtui::Core
    Qtui::Controls
)

if(TARGET Qt6::Multimedia)
    target_link_libraries(qtui_cyber_audio_module PUBLIC Qt6::Multimedia)
    target_compile_definitions(qtui_cyber_audio_module PUBLIC CYBER_AUDIO_HAVE_MULTIMEDIA=1)
    message(STATUS "cyber_audio: Qt Multimedia found, file playback and microphone enabled")
else()
    target_compile_definitions(qtui_cyber_audio_module PUBLIC CYBER_AUDIO_HAVE_MULTIMEDIA=0)
    message(STATUS "cyber_audio: Qt Multimedia not found, demo source only")
endif()

# QML module libraries do not make their runtime plugins build dependencies.
foreach(plugin_target IN ITEMS
        QtuiCoreplugin QtuiControlsplugin Qtui::CorePlugin Qtui::ControlsPlugin)
    if(TARGET ${plugin_target})
        add_dependencies(qtui_cyber_audio_module ${plugin_target})
    endif()
endforeach()

qt_add_executable(qtui_cyber_audio main.cpp)
# Link the module library as well as its plugin so the executables inherit the
# module's include path and the CYBER_AUDIO_HAVE_MULTIMEDIA definition.
target_link_libraries(qtui_cyber_audio PRIVATE
    Qt6::Core
    Qt6::Qml
    qtui_cyber_audio_module
    qtui_cyber_audio_moduleplugin
)

if(TARGET Qtui::TestRunner)
    qt_add_executable(qtui_cyber_audio_test main_test.cpp)
    target_link_libraries(qtui_cyber_audio_test PRIVATE
        Qt6::Core
        Qt6::Qml
        Qtui::Core
        Qtui::TestRunner
        qtui_cyber_audio_module
        qtui_cyber_audio_moduleplugin
    )

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
    qt_add_executable(test_cyber_audio_dsp test_cyber_audio_dsp.cpp)
    target_link_libraries(test_cyber_audio_dsp PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qtui::Core
        qtui_cyber_audio_module
    )
    add_test(NAME cyber_audio_dsp COMMAND test_cyber_audio_dsp)
    set_tests_properties(cyber_audio_dsp PROPERTIES LABELS AgentSmoke)
endif()
