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
    DEPENDENCIES TARGET QtuiCore QtuiControls QtuiWidgets
)

target_link_libraries(qtui_pos_cashier PRIVATE Qt6::Core Qt6::Qml QtuiCore QtuiControls QtuiWidgets)
