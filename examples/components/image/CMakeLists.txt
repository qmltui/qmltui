qt_add_executable(qtui_image_demo main.cpp)

qt_add_qml_module(qtui_image_demo
    URI ImageDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    RESOURCES
        demo_image.png
        image2.jpg
        panda.png
        map.png
        qt_logo_icon_248726.webp
        
    DEPENDENCIES TARGET
        Qtui::Core
        Qtui::Controls
        Qtui::Image
)

target_link_libraries(qtui_image_demo PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qtui::Core Qtui::Controls Qtui::Image)
