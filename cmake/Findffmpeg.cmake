set(ffmpeg_path $ENV{FFMPEG_PATH})

set(prefix "${ffmpeg_path}")
set(exec_prefix "${prefix}")
set(libdir "${exec_prefix}/lib")
set(FFMPEG_PREFIX "${ffmpeg_path}")
set(FFMPEG_EXEC_PREFIX "${ffmpeg_path}")
set(FFMPEG_LIBDIR "${exec_prefix}/lib")
set(FFMPEG_INCLUDE_DIRS "${prefix}/include")
set(FFMPEG_LIBRARIES "-L${FFMPEG_LIBDIR} -lavcodec -lavformat -lavdevice -lswresample -lavfilter -lavutil -lm")
if (NOT WIN32)
    set(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} "-lz -lavresample -lpostproc")
endif()
message(${FFMPEG_LIBRARIES} ${FFMPEG_INCLUDE_DIRS})
string(STRIP "${FFMPEG_LIBRARIES}" FFMPEG_LIBRARIES)