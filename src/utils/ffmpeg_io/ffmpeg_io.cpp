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
#include <libavdevice/avdevice.h>
}

namespace {
  std::string ffmpeg_error2str(int err){
    static char msg[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(msg, AV_ERROR_MAX_STRING_SIZE, err);
  }
}

srp::ffmpeg_io_container::ffmpeg_io_container(io_device source, Mode mode) : mode_{mode}, source_(std::move(source)) {
  avdevice_register_all();
  if (mode_ == Mode::read) {
    open_exist(source_.name, source_.format);
  }
  else
    create(source_.name);
}

srp::ffmpeg_io_container::~ffmpeg_io_container() {
  if (context_ptr_) {

    if (mode_ == Mode::write)
      av_write_trailer(context_ptr_);
    else {
      avformat_close_input(&context_ptr_);
    }

    avformat_free_context(context_ptr_);
    context_ptr_ = nullptr;
  }
}

unsigned srp::ffmpeg_io_container::streams_count() const { return (context_ptr_) ? context_ptr_->nb_streams : 0; }

std::unique_ptr<srp::ffmpeg_io_container::ffmpeg_stream> srp::ffmpeg_io_container::open_stream(unsigned int stream_index) {
  if (stream_index >= context_ptr_->nb_streams)
    throw std::out_of_range(std::to_string(stream_index) + " out of range " + std::to_string(streams_count()));
  return std::make_unique<srp::ffmpeg_io_container::ffmpeg_stream>(this->context_ptr_, context_ptr_->streams[stream_index], stream_index);
}

void srp::ffmpeg_io_container::open_exist(std::string const &source, std::string const &format) {
  context_ptr_ = avformat_alloc_context();

  if (context_ptr_ == nullptr) {
    LOGE << "Fail to allocate context.";
    throw context_allocation_fail();
  }

  auto *ifmt = av_find_input_format(format.c_str());

  auto ecode = avformat_open_input(&context_ptr_, source.c_str(), ifmt, nullptr);
  if (ecode != 0) {
    LOGE << "Fail to open context for source " << source << ". Error code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
    throw context_open_fail(ecode);
  } else {
    LOGD << "Open container with format " << context_ptr_->iformat->long_name;
  }

  ecode = avformat_find_stream_info(context_ptr_, nullptr);
  if (ecode < 0) {
    LOGW << "Unable to find stream info. Error code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
  } else {
    LOGD << "Find " << context_ptr_->nb_streams << " streams.";
  }
}

void srp::ffmpeg_io_container::create(const std::string &source) {
  auto ecode = avformat_alloc_output_context2(&context_ptr_, nullptr, nullptr, source.c_str());
  if (ecode < 0) {
    LOGE << "Fail to allocate output context. Code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
    throw context_allocation_fail();
  }

  ecode = avio_open(&context_ptr_->pb, source.c_str(), AVIO_FLAG_WRITE);
  if (ecode < 0) {
    LOGE << "Fail to open output file. " << source << " Code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
    throw std::runtime_error("Fail to open output.");
  }
}
std::unique_ptr<srp::ffmpeg_io_container::ffmpeg_stream>
srp::ffmpeg_io_container::create_stream(const srp::ffmpeg_io_container::ffmpeg_stream::stream_options &option) {
  auto stream = std::make_unique<ffmpeg_stream>(context_ptr_, option);

  av_dump_format(context_ptr_, 0, source_.name.c_str(), 1);
  AVDictionary *opt = nullptr;
  auto ecode = avformat_write_header(context_ptr_, &opt);
  if (ecode < 0) { LOGW << "Error in write header. Code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode); }

  return stream;
}

srp::ffmpeg_io_container::ffmpeg_stream::ffmpeg_stream(AVFormatContext *linked_context, AVStream *linked_stream, size_t stream_index)
    : linked_context_{linked_context}, stream_{linked_stream}, stream_index_{stream_index} {
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
    LOGE << "Fail to open codec. Error code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
    throw codec_open_fail();
  }

  packet_ = av_packet_alloc();
  if (packet_ == nullptr) {
    LOGE << "Fail to alloc packet.";
    throw std::runtime_error("Packet allocation error");
  }
}


srp::ffmpeg_io_container::ffmpeg_stream::~ffmpeg_stream() {
  if (is_writable_)
    write_frame(nullptr);
  if (coder_context_) avcodec_free_context(&coder_context_);

  if (!is_writable_)
    av_packet_free(&packet_);
}

bool srp::ffmpeg_io_container::ffmpeg_stream::extract_frame(AVFrame *frame) {
  int read_code;

  while ((read_code = avcodec_receive_frame(coder_context_, frame)) == AVERROR(EAGAIN)) {
    [[maybe_unused]] auto ecode = av_read_frame(linked_context_, packet_);
    if (ecode < 0)
      ecode = avcodec_send_packet(coder_context_, nullptr);
    else
      ecode = avcodec_send_packet(coder_context_, packet_);
    av_packet_unref(packet_);
  }

  return read_code == AVERROR_EOF;// || frame->pts < 0;
}

srp::ffmpeg_io_container::ffmpeg_stream::ffmpeg_stream(AVFormatContext *linked_context, stream_options const &options)
    : linked_context_{linked_context}, opt_{options}, is_writable_{true} {
  if (opt_.type == stream_options::stream_type::audio) create_audio(options);

  stream_index_ = linked_context_->nb_streams - 1;

  AVDictionary *opt = nullptr;
  auto ecode = avcodec_open2(coder_context_, codec_, &opt);
  if (ecode < 0) {
    LOGE << "Fail to open encoder codec. Code: " << ecode << ". Detail: " << ffmpeg_error2str(ecode);
    throw codec_open_fail();
  }

  avcodec_parameters_from_context(stream_->codecpar, coder_context_);
  codec_par_ = stream_->codecpar;
}

void srp::ffmpeg_io_container::ffmpeg_stream::create_audio(const srp::ffmpeg_io_container::ffmpeg_stream::stream_options &options) {
  auto &format = linked_context_->oformat;

  codec_ = avcodec_find_encoder(format->audio_codec);
  if (codec_ == nullptr) {
    LOGE << "Could not find encoder for " << avcodec_get_name(format->audio_codec);
    throw std::runtime_error("fail to find encoder");
  }

  stream_ = avformat_new_stream(linked_context_, codec_);
  if (stream_ == nullptr) {
    LOGE << "Fail to create stream.";
    throw std::runtime_error("stream creation error");
  }

  coder_context_ = avcodec_alloc_context3(codec_);
  if (coder_context_ == nullptr) {
    LOGE << "Fail to allocate encoder context.";
    throw codec_open_fail();
  }

  coder_context_->sample_fmt = (codec_->sample_fmts) ? codec_->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;

  coder_context_->bit_rate = opt_.bitrate;
  coder_context_->sample_rate = opt_.audio_opt.sample_rate;

  if (codec_->supported_samplerates) {
    coder_context_->sample_rate = codec_->supported_samplerates[0];
    for (auto i = 0; codec_->supported_samplerates[i]; i++) {
      if (codec_->supported_samplerates[i] == opt_.audio_opt.sample_rate) coder_context_->sample_rate = opt_.audio_opt.sample_rate;
    }
  }

  coder_context_->channels = av_get_channel_layout_nb_channels(coder_context_->channel_layout);
  coder_context_->channel_layout = AV_CH_LAYOUT_STEREO;
  if (codec_->channel_layouts) {
    coder_context_->channel_layout = codec_->channel_layouts[0];
    for (auto i = 0; codec_->channel_layouts[i]; i++) {
      if (codec_->channel_layouts[i] == AV_CH_LAYOUT_STEREO) coder_context_->channel_layout = AV_CH_LAYOUT_STEREO;
    }
  }
  coder_context_->channels = av_get_channel_layout_nb_channels(coder_context_->channel_layout);
  stream_->time_base.num = 1;
  stream_->time_base.den = coder_context_->sample_rate;

  if (linked_context_->oformat->flags & AVFMT_GLOBALHEADER)
    coder_context_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

bool srp::ffmpeg_io_container::ffmpeg_stream::write_frame(const AVFrame *frame) {
  AVPacket packet;

  av_init_packet(&packet);
  auto ecode = avcodec_send_frame(coder_context_, frame);

  if (ecode < 0) {
    LOGE << "Error in send frame to coder.";
    //throw std::runtime_error("encoding error");
  }

  while ((ecode = avcodec_receive_packet(coder_context_, &packet)) >= 0) {
    packet.stream_index = stream_index_;
    av_packet_rescale_ts(&packet, coder_context_->time_base, stream_->time_base);
    //    packet.dts = dts;
    //    packet.pts = frame->pts;
    av_interleaved_write_frame(linked_context_, &packet);
  }

  return true;
}

srp::audio_frame::audio_frame(AVFrame *raw_frame)
    : nb_samples{raw_frame->nb_samples}, format{raw_frame->format}, sample_rate{raw_frame->sample_rate}, pts{raw_frame->pts} {
  data.resize(raw_frame->channels);

  for (auto channel_id = 0; channel_id < raw_frame->channels; ++channel_id) {
    data[channel_id].resize(raw_frame->linesize[0]);
    std::copy(raw_frame->extended_data[channel_id], raw_frame->extended_data[channel_id] + raw_frame->linesize[0], data[channel_id].begin());
  }
}

srp::native_audio_frame::native_audio_frame(AVFrame *raw) {
  frame = av_frame_clone(raw);
}
srp::native_audio_frame::~native_audio_frame() { release(); }
void srp::native_audio_frame::release() {
  if (frame) {
    av_frame_unref(frame);
    frame = nullptr;
  }
}
AVFrame *srp::native_audio_frame::to_avframe(size_t pts) const {
  AVFrame *f = av_frame_clone(frame);
  f->pts = pts;
  return f;
}
long long srp::native_audio_frame::pts_delta(const srp::native_audio_frame &rhs) const {
  return rhs.frame->pts - frame->pts;
}
