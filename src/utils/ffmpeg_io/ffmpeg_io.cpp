//
// Created by yalavrinenko on 14.01.2021.
//
#include "ffmpeg_io.hpp"
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

srp::ffmpeg_io_container::ffmpeg_io_container(std::string source, Mode mode): mode_{mode}, source_(std::move(source)) {
  if (mode_ == Mode::read)
    open_exist(source_);
}

srp::ffmpeg_io_container::~ffmpeg_io_container() {
  if (mode_ == Mode::write){
    av_dump_format(context_ptr_, 0, source_.c_str(), 1);
    AVDictionary *opt = nullptr;
    auto ecode = avformat_write_header(context_ptr_, &opt);
    if (ecode < 0) {
      LOGW << "Error in write header. Code:" << ecode;
    }
  }
  if (context_ptr_) {
    avformat_free_context(context_ptr_);
    context_ptr_ = nullptr;
  }
}

unsigned srp::ffmpeg_io_container::streams_count() const { return (context_ptr_) ? context_ptr_->nb_streams : 0; }

std::unique_ptr<srp::ffmpeg_io_container::ffmpeg_stream> srp::ffmpeg_io_container::open_stream(unsigned int stream_index) {
  if (stream_index >= context_ptr_->nb_streams)
    throw std::out_of_range(std::to_string(stream_index) + " out of range " + std::to_string(streams_count()));
  return std::make_unique<srp::ffmpeg_io_container::ffmpeg_stream>(this->context_ptr_, context_ptr_->streams[stream_index]);
}

void srp::ffmpeg_io_container::open_exist(std::string const &source) {
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

srp::ffmpeg_io_container::ffmpeg_stream::ffmpeg_stream(AVFormatContext *linked_context, AVStream *linked_stream)
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
  if (coder_context_ == nullptr) {
    LOGE << "Unable to open codec context.";
    throw codec_open_fail();
  }

  avcodec_parameters_to_context(coder_context_, codec_par_);
  auto ecode = avcodec_open2(coder_context_, codec_, nullptr);
  if (ecode != 0) {
    LOGE << "Fail to open codec. Error code: " << ecode;
    throw codec_open_fail();
  }

  packet_ = av_packet_alloc();
  if (packet_ == nullptr) {
    LOGE << "Fail to alloc packet.";
    throw std::runtime_error("Packet allocation error");
  }
}


srp::ffmpeg_io_container::ffmpeg_stream::~ffmpeg_stream() {
  if (coder_context_) avcodec_free_context(&coder_context_);
}

bool srp::ffmpeg_io_container::ffmpeg_stream::extract_frame(AVFrame *frame) {

  auto read_code = avcodec_receive_frame(coder_context_, frame);

  while (read_code == AVERROR(EAGAIN)) {
    [[maybe_unused]] auto ecode = av_read_frame(linked_context_, packet_);
    ecode = avcodec_send_packet(coder_context_, packet_);
    read_code = avcodec_receive_frame(coder_context_, frame);
  }

  return read_code == AVERROR_EOF;
}

srp::audio_frame::audio_frame(AVFrame *raw_frame)
    : nb_samples{raw_frame->nb_samples}, format{raw_frame->format}, sample_rate{raw_frame->sample_rate}, pts{raw_frame->pts} {
  data.resize(raw_frame->channels);

  for (auto channel_id = 0; channel_id < raw_frame->channels; ++channel_id) {
    data[channel_id].resize(raw_frame->linesize[0]);
    std::copy(raw_frame->extended_data[channel_id], raw_frame->extended_data[channel_id] + raw_frame->linesize[0], data[channel_id].begin());
  }
}
