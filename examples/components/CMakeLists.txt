# Reusable QML recipes, C++ custom items/models, and PTY host examples.
# Built-in QmlTui, QmlTui.Controls, and QmlTui.Image types live in
# examples/gallery/examples/*.qml and run via `qmltui_gallery --example <Name>`.
add_subdirectory(big_text)
add_subdirectory(calendar)
add_subdirectory(standard_item_delegate)
add_subdirectory(status_badge)
add_subdirectory(table_view)
add_subdirectory(text_area_highlighting)
if(QMLTUI_ENABLE_TERMINAL_HOST)
    add_subdirectory(terminal_view)
endif()
