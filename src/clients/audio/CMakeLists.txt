cmake_minimum_required(VERSION 3.17)
project(SRP)

add_library(audio_instance audio_instance.cpp audio_instance.hpp)
target_link_libraries(audio_instance ffmpeg_io)
target_include_directories(audio_instance PUBLIC "${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}"
        "${Protobuf_INCLUDE_DIR}")