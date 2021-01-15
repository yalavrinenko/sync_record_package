//
// Created by yalavrinenko on 15.01.2021.
//

#include "ffmpeg_writer.hpp"
srp::ffmpeg_writer::ffmpeg_writer(const std::string &source): container_(source, ffmpeg_io_container::Mode::write) {
}
void srp::ffmpeg_writer::create_stream(const srp::ffmpeg_io_container::ffmpeg_stream::stream_options &options) {
  stream_ = container_.create_stream(options);
}
