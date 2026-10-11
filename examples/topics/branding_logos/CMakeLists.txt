qt_add_executable(qamui_branding_logos_demo
    main.cpp
    BrandingCatalog.h
    BrandingCatalog.cpp
)

qt_add_qml_module(qamui_branding_logos_demo
    URI BrandingLogosDemo
    VERSION 1.0
    QML_FILES
        Main.qml
    DEPENDENCIES TARGET
        Qamui::Core
        Qamui::Controls
        Qamui::Image
)

set(BRANDING_FILES
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark_light.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark_mono.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark_tile.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_lockup_dark.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_lockup_light.svg"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark_cells.png"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_logo_sheet.png"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/qamui_mark_cells.txt"
    "${PROJECT_SOURCE_DIR}/docs/assets/branding/mascot_ascii.txt"
)

foreach(bfile IN LISTS BRANDING_FILES)
    get_filename_component(bname "${bfile}" NAME)
    set_source_files_properties("${bfile}" PROPERTIES QT_RESOURCE_ALIAS "${bname}")
endforeach()

qt_add_resources(qamui_branding_logos_demo "branding"
    PREFIX "/branding"
    FILES ${BRANDING_FILES}
)

target_link_libraries(qamui_branding_logos_demo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qamui::Core
    Qamui::Controls
    Qamui::Image
)

if(TARGET Qamui::Runner)
    target_link_libraries(qamui_branding_logos_demo PRIVATE Qamui::Runner)
    qamui_add_scenario(
        NAME branding_logos_smoke
        TARGET qamui_branding_logos_demo
        SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/test_smoke.js"
        COLUMNS 100
        ROWS 30
        CLOCK VIRTUAL
        LABELS AgentSmoke
    )
endif()
