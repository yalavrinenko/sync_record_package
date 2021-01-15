//
// Created by yalavrinenko on 14.01.2021.
//

#ifndef SRP_FFMPEG_IO_HPP
#define SRP_FFMPEG_IO_HPP
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

struct AVFormatContext;
struct AVCodecParameters;
struct AVStream;
struct AVCodec;
struct AVCodecContext;
struct AVPacket;
struct AVFrame;

namespace srp {
  struct audio_frame {
    audio_frame() = default;
    explicit audio_frame(AVFrame* raw_frame);

    using samples = std::vector<uint8_t>;
    std::vector<samples> data{};
    int nb_samples{};
    int format{};
    int sample_rate{};
    int64_t pts{};
  };

  class video_frame {};

  class ffmpeg_io_container {
  public:
    enum class Mode{
      read,
      write
    };
    class ffmpeg_stream {
    public:
      ffmpeg_stream() = default;
      ffmpeg_stream(AVFormatContext *linked_context, AVStream *linked_stream);

      bool extract_frame(AVFrame *frame);

      ~ffmpeg_stream();

    protected:
      AVFormatContext *linked_context_{};
      AVCodecParameters *codec_par_ = nullptr;
      AVStream *stream_ = nullptr;
      AVCodec *codec_ = nullptr;
      AVCodecContext *coder_context_ = nullptr;
      AVPacket *packet_ = nullptr;
    };

    explicit ffmpeg_io_container(std::string source, Mode mode);

    [[nodiscard]] unsigned streams_count() const;

    std::unique_ptr<ffmpeg_stream> open_stream(unsigned int stream_index);

    ~ffmpeg_io_container();

  private:
    void open_exist(std::string const &source);

    Mode mode_;

    std::string source_;

    AVFormatContext *context_ptr_ = nullptr;
  };
}// namespace srp
#endif//SRP_FFMPEG_IO_HPP
