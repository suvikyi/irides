cmake_minimum_required(VERSION 3.21)

project(irides VERSION 0.2.3 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Gui WaylandClient WaylandClientPrivate)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

set(IRIDES_PROTOCOL_OUT "${CMAKE_CURRENT_BINARY_DIR}/protocols")
add_custom_command(
    OUTPUT
        "${IRIDES_PROTOCOL_OUT}/hyprion-grade-v1-client-protocol.h"
        "${IRIDES_PROTOCOL_OUT}/hyprion-grade-v1-protocol.c"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${IRIDES_PROTOCOL_OUT}"
    COMMAND "${WAYLAND_SCANNER}" client-header "${CMAKE_CURRENT_SOURCE_DIR}/protocols/hyprion-grade-v1.xml" "${IRIDES_PROTOCOL_OUT}/hyprion-grade-v1-client-protocol.h"
    COMMAND "${WAYLAND_SCANNER}" private-code "${CMAKE_CURRENT_SOURCE_DIR}/protocols/hyprion-grade-v1.xml" "${IRIDES_PROTOCOL_OUT}/hyprion-grade-v1-protocol.c"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/protocols/hyprion-grade-v1.xml"
    VERBATIM
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")

qt_add_library(irides SHARED
    hypriongrade.cpp
    hypriongrade.hpp
    "${IRIDES_PROTOCOL_OUT}/hyprion-grade-v1-protocol.c"
)

set_source_files_properties(Oklab.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(irides
    URI Irides
    VERSION 0.2
    QML_FILES
        Grade.qml
        Oklab.qml
        Shadow.qml
    RESOURCES
        grade.frag.qsb
        shadow.frag.qsb
)

target_include_directories(irides PRIVATE "${IRIDES_PROTOCOL_OUT}")
target_link_libraries(irides PRIVATE
    Qt6::Quick
    Qt6::Gui
    Qt6::WaylandClient
    Qt6::WaylandClientPrivate
    wayland-client
)
