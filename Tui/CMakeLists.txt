# The library backing the QML module
qt_add_qml_module(Qtui
    URI Tui
    VERSION 1.0
    SOURCES
        src/core/ColorMapper.h
        src/core/ColorMapper.cpp
        src/core/InputDecoder.cpp
        src/core/Cell.h
        src/core/TextGridBuffer.h
        src/core/TextGridBuffer.cpp
        src/core/TerminalDevice.h
        src/core/TerminalDevice.cpp
        src/engine/TerminalEngine.h
        src/engine/TerminalEngine.cpp
        src/engine/Renderer.h
        src/engine/FrameRenderer.h
        src/engine/FrameRenderer.cpp
        src/engine/StreamRenderer.h
        src/engine/StreamRenderer.cpp
        src/engine/KeyEvent.h
        src/engine/MouseEvent.h
        src/engine/Keys.h
        src/engine/Animation.h
        src/engine/Animation.cpp
        src/engine/Behavior.h
        src/engine/Behavior.cpp
        src/engine/State.h
        src/engine/State.cpp
        src/engine/Transition.h
        src/engine/Transition.cpp
        src/items/Item.h
        src/items/Item.cpp
        src/items/Rectangle.h
        src/items/Rectangle.cpp
        src/items/Anchors.h
        src/items/Anchors.cpp
        src/items/AnchorLine.h
        src/items/Border.h
        src/items/Border.cpp
        src/items/TextInput.h
        src/items/TextInput.cpp
        src/items/TextArea.h
        src/items/TextArea.cpp
        src/items/ScrollView.h
        src/items/ScrollView.cpp
        src/items/Terminal.h
        src/items/Terminal.cpp
        src/items/Popup.h
        src/items/Popup.cpp
        src/items/FlexLayout.h
        src/items/FlexLayout.cpp
        src/items/Text.h
        src/items/Text.cpp
        src/items/StreamAppender.h
        src/items/StreamAppender.cpp
        src/items/Row.h
        src/items/Row.cpp
        src/items/Column.h
        src/items/Column.cpp
        src/items/MouseArea.h
        src/items/MouseArea.cpp
        src/items/Repeater.h
        src/items/Repeater.cpp
        src/items/ListView.h
        src/items/ListView.cpp
        src/items/GridView.h
        src/items/GridView.cpp
        src/items/BrailleCanvas.h
        src/items/BrailleCanvas.cpp
)

target_include_directories(Qtui PUBLIC src/items src/core src/engine src)

target_link_libraries(Qtui PUBLIC
    Qt6::Qml
)
target_link_libraries(Qtui PRIVATE
    Qt6::QmlPrivate
)

qt_add_qml_module(TuiControls
    URI Tui.Controls
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Controls"
    QML_FILES
        src/controls/Button.qml
    DEPENDENCIES TARGET Qtui
)

target_link_libraries(TuiControls PRIVATE Qt6::Core Qt6::Qml Qtui)
