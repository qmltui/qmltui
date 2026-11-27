# Reusable QML recipes and C++ custom items/models.
# Built-in QmlTui, QmlTui.Controls, and QmlTui.Image types
# live in examples/gallery/examples/*.qml and run via `qmltui_gallery --example <Name>`.
add_subdirectory(big_text)
add_subdirectory(calendar)
add_subdirectory(cpp_only)
add_subdirectory(custom_cpp_item)
if(QMLTUI_ENABLE_PRIVATE_QT)
    add_subdirectory(independent_states)
endif()
add_subdirectory(inline_repl)
add_subdirectory(lazy_model)
if(QMLTUI_ENABLE_IMAGE)
    add_subdirectory(live_image)
endif()
add_subdirectory(standard_item_delegate)
add_subdirectory(status_badge)
add_subdirectory(table_view)
add_subdirectory(text_area_highlighting)
