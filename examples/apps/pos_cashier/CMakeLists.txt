qt_add_executable(qamui_pos_cashier main.cpp)

qt_add_qml_module(qamui_pos_cashier
    URI POSCashier
    VERSION 1.0
    QML_FILES
        Main.qml
        CatalogView.qml
        CartView.qml
        Keypad.qml
        PaymentView.qml
        SuccessView.qml
 DEPENDENCIES TARGET Qamui::Core Qamui::Controls
)

target_link_libraries(qamui_pos_cashier PRIVATE Qt6::Core Qt6::Qml Qamui::Core Qamui::Controls Qamui::Controls)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_pos_cashier PRIVATE Qamui::Runner)
endif()

if(QAMUI_INSTALL_EXAMPLES)
    install(TARGETS qamui_pos_cashier RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
