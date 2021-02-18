cmake_minimum_required(VERSION 3.17)
project(SRP)

find_package(Boost COMPONENTS log log_setup REQUIRED)
find_package(Qt5 COMPONENTS Widgets Qml Quick REQUIRED)

qt5_add_resources(QT_RESOURCES ui.qrc)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(ui_control ui_back.cpp ui_back.hpp
        ui_control_client.cpp ui_control_client.hpp UiAppCore.cpp UiAppCore.hpp)
target_compile_definitions(ui_control PUBLIC ${Qt5Widgets_DEFINITIONS} ${QtQml_DEFINITIONS} ${${Qt5Quick_DEFINITIONS}})
target_include_directories(ui_control PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}"
        "${Qt5Widgets_INCLUDE_DIRS}" "${QtQml_INCLUDE_DIRS}" "${Protobuf_INCLUDE_DIR}")
#target_link_libraries(ui_control Boost::log Boost::log_setup)
target_link_libraries(ui_control Boost::log Boost::log_setup Qt5::Widgets Qt5::Qml Qt5::Quick)
target_compile_definitions(ui_control PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

configure_file(ui.qml ${CMAKE_BINARY_DIR}/ui.qml COPYONLY)