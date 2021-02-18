cmake_minimum_required(VERSION 3.17)
project(SRP)

find_package(SFML 2 REQUIRED COMPONENTS graphics window system)
find_package(ImGui-SFML REQUIRED)
find_package(Boost COMPONENTS log log_setup REQUIRED)

add_library(ui_control gui_entries.cpp gui_input.cpp guilogger.cpp ../control/ui_control_client.cpp sfml_control_instance.cpp sfml_control_instance.hpp common_info_window.cpp common_info_window.hpp device_info_window.cpp device_info_window.hpp)
target_include_directories(ui_control PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}")

target_link_libraries(ui_control Boost::log Boost::log_setup
        sfml-graphics sfml-window sfml-system ImGui-SFML::ImGui-SFML)
target_compile_definitions(ui_control PUBLIC "-DBOOST_ALL_DYN_LINK -DBOOST_LOG_USE_CHAR")

