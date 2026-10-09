qt_add_executable(qtui_pos_cashier main.cpp)

qt_add_qml_module(qtui_pos_cashier
    URI POSCashier
    VERSION 1.0
    QML_FILES
        Main.qml
        CatalogView.qml
        CartView.qml
        Keypad.qml
        PaymentView.qml
        SuccessView.qml
 DEPENDENCIES TARGET Qtui::Core Qtui::Controls
)

target_link_libraries(qtui_pos_cashier PRIVATE Qt6::Core Qt6::Qml Qtui::Core Qtui::Controls Qtui::Controls)

if(TARGET Qtui::Runner)
    target_link_libraries(qtui_pos_cashier PRIVATE Qtui::Runner)
endif()

if(QTUI_INSTALL_EXAMPLES)
    install(TARGETS qtui_pos_cashier RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
