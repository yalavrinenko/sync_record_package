//
// Created by yalavrinenko on 15.01.2021.
//

#include "ffmpeg_writer.hpp"
#include "../logger.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
srp::ffmpeg_writer::ffmpeg_writer(const std::string &source): container_({source, ""}, ffmpeg_io_container::Mode::write) {
  auto writer_function = [this]() {
    auto frame_count = 0ul;
    while (be_active_){
      frame_queue_.wait_for_data();

      while (!frame_queue_.empty()) {
        auto pframe = frame_queue_.front();

        stream_->write_frame(pframe.frame);

        ++frame_count;
        pframe.release();
        frame_queue_.pop();
      }
    }

    return frame_count;
  };
  be_active_ = true;
  writer_thread_ = std::async(std::launch::async, writer_function);
}
void srp::ffmpeg_writer::create_stream(const srp::ffmpeg_io_container::ffmpeg_stream::stream_options &options) {
  stream_ = container_.create_stream(options);
}
void srp::ffmpeg_writer::dequeue_frame(const srp::ffmpeg_writer::packed_frame& pframe) {
  frame_queue_.push(pframe);
}
srp::ffmpeg_writer::~ffmpeg_writer() {
  be_active_ = false;
  frame_queue_.kill();
  auto writen = writer_thread_.get();
  LOGD << "Write " << writen << " frames to " << stream_->context()->url;
}
void srp::ffmpeg_writer::packed_frame::release() {
  av_frame_free(&frame);
}
