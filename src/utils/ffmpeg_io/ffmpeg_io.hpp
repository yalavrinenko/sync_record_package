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
  struct native_audio_frame{
    native_audio_frame() = default;
    explicit native_audio_frame(AVFrame* raw);
    native_audio_frame(native_audio_frame&& rhs) noexcept{
      frame = rhs.frame; rhs.frame = nullptr;
    }

    native_audio_frame& operator= (native_audio_frame&& rhs) noexcept{
      release();
      frame = rhs.frame; rhs.frame = nullptr;
      return *this;
    }

    AVFrame* to_avframe(size_t pts) const;

    ~native_audio_frame();

    AVFrame* frame = nullptr;
  private:
    void release();
  };

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
      struct stream_options{
        enum class stream_type{
          audio,
          video,
          text
        };

        stream_type type{};
        size_t bitrate = 64000;

        struct audio{
          int sample_rate = 44100;
        };

        size_t pts_step = 1;

        audio audio_opt;
      };

      ffmpeg_stream() = default;
      ffmpeg_stream(AVFormatContext *linked_context, AVStream *linked_stream, size_t stream_index);
      ffmpeg_stream(AVFormatContext *linked_context, stream_options const& options);

      bool extract_frame(AVFrame *frame);
      bool write_frame(const AVFrame *frame);

      [[nodiscard]] auto context() const {
        return linked_context_;
      }

      auto const& option() const {
        return opt_;
      }

      ~ffmpeg_stream();

    private:
      void create_audio(stream_options const &options);

      AVFormatContext *linked_context_{};
      AVCodecParameters *codec_par_ = nullptr;
      AVStream *stream_ = nullptr;
      AVCodec *codec_ = nullptr;
      AVCodecContext *coder_context_ = nullptr;
      AVPacket *packet_ = nullptr;
      stream_options opt_{};
      size_t stream_index_{};
      bool is_writable_ {false};
    };

    struct io_device{
      std::string name;
      std::string format;
    };

    ffmpeg_io_container(io_device source, Mode mode);

    [[nodiscard]] unsigned streams_count() const;

    std::unique_ptr<ffmpeg_stream> open_stream(unsigned int stream_index);
    std::unique_ptr<ffmpeg_stream> create_stream(ffmpeg_stream::stream_options const &option);

    ~ffmpeg_io_container();

  private:
    void open_exist(std::string const &source, std::string const &format);

    void create(std::string const &source);

    Mode mode_;

    io_device source_;

    AVFormatContext *context_ptr_ = nullptr;
  };
}// namespace srp
#endif//SRP_FFMPEG_IO_HPP
