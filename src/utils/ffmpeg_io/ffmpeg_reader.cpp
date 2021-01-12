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

srp::ffmpeg_reader::ffmpeg_reader(const std::string &source) : container_(source) {}

srp::ffmpeg_io_container::ffmpeg_io_container(const std::string &source) {
  context_ptr_ = avformat_alloc_context();

  if (context_ptr_ == nullptr) {
    LOGE << "Fail to allocate context.";
    throw context_allocation_fail();
  }

  auto ecode = avformat_open_input(&context_ptr_, source.c_str(), nullptr, nullptr);
  if (ecode != 0) {
    LOGE << "Fail to open context for source " << source << ". Error code: " << ecode;
    throw context_open_fail(ecode);
  } else {
    LOGD << "Open container with format " << context_ptr_->iformat->long_name;
  }

  ecode = avformat_find_stream_info(context_ptr_, nullptr);
  if (ecode < 0) {
    LOGW << "Unable to find stream info. Error code: " << ecode;
  } else {
    LOGD << "Find " << context_ptr_->nb_streams << " streams.";
  }
}

srp::ffmpeg_io_container::~ffmpeg_io_container() {
  if (context_ptr_) {
    avformat_free_context(context_ptr_);
    context_ptr_ = nullptr;
  }
}

unsigned srp::ffmpeg_io_container::streams_count() const { return (context_ptr_) ? context_ptr_->nb_streams : 0; }

std::unique_ptr<srp::ffmpeg_io_container::ffmpeg_stream> srp::ffmpeg_io_container::open_stream(unsigned int stream_index) {
  if (stream_index < context_ptr_->nb_streams)
    throw std::out_of_range(std::to_string(stream_index) + " out of range " + std::to_string(streams_count()));
  return std::make_unique<srp::ffmpeg_io_container::ffmpeg_stream>(this->context_ptr_, context_ptr_->streams[stream_index]);
}

srp::ffmpeg_io_container::ffmpeg_stream::ffmpeg_stream(const AVFormatContext *linked_context, AVStream *linked_stream)
    : linked_context_{linked_context}, stream_{linked_stream} {
  codec_par_ = stream_->codecpar;

  codec_ = avcodec_find_decoder(codec_par_->codec_id);
  if (codec_ == nullptr) {
    LOGE << "Unable to find decoder for codec_id " << codec_par_->codec_id;
    throw codec_open_fail();
  } else {
    if (codec_par_->codec_type == AVMEDIA_TYPE_VIDEO) {
      LOGD << "Video Codec: resolution " << codec_par_->width << "x" << codec_par_->height;
    } else if (codec_par_->codec_type == AVMEDIA_TYPE_AUDIO) {
      LOGD << "Audio Codec: " << codec_par_->channels << " channels, sample rate " << codec_par_->sample_rate;
    }
    LOGD << "Codec " << codec_->long_name << " ID " << codec_->id << " bit_rate " << codec_par_->bit_rate;
  }

  coder_context_ = avcodec_alloc_context3(codec_);
  if (coder_context_ == nullptr){
    LOGE << "Unable to open codec context.";
    throw codec_open_fail();
  }

  avcodec_parameters_to_context(coder_context_, codec_par_);
  auto ecode = avcodec_open2(coder_context_, codec_, nullptr);
  if (ecode != 0){
    LOGE << "Fail to open codec. Error code: " << ecode;
    throw codec_open_fail();
  }
}


srp::ffmpeg_io_container::ffmpeg_stream::~ffmpeg_stream() {
  if (coder_context_) avcodec_free_context(&coder_context_);
}
