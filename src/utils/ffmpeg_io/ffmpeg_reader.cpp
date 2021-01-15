//
// Created by yalavrinenko on 12.01.2021.
//

#include "ffmpeg_reader.hpp"
#include "../logger.hpp"
#include "exceptions.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

srp::ffmpeg_reader::ffmpeg_reader(const std::string &source) : container_(source, ffmpeg_io_container::Mode::read) {
  local_frame_ = av_frame_alloc();
}
void srp::ffmpeg_reader::select_stream(unsigned stream_id) { stream_ = container_.open_stream(stream_id); }
