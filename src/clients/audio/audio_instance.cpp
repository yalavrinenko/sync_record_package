//
// Created by yalavrinenko on 12.01.2021.
//

#include "audio_instance.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <utils/ffmpeg_io/ffmpeg_reader.hpp>
#include <utils/ffmpeg_io/ffmpeg_writer.hpp>
#include <utils/logger.hpp>
#include <utils/io.hpp>

using namespace srp;

class audio_instance::audio_io {
public:
  explicit audio_io(AudioCaptureOptions options) : options_(std::move(options)) {}

  struct recording_info {
    size_t frames;
    std::chrono::duration<double> duration;
  };

  struct timestamp_entry {
    size_t frame_id;
    std::chrono::duration<double> ts;
    double fps;
  };

  using timestamp = srp::IoUtils::tsafe_timestamp<timestamp_entry>;

  struct io_block {
    io_block(std::string const &format, std::string const &device, std::string const &outpath, std::string const &stampspath,
             AudioCaptureOptions const &copt)
        : input({device, format}), output(outpath), timestamps(stampspath) {
      input.select_stream(copt.input_stream_index());

      srp::native_audio_frame frame1, frame2;
      input >> frame1 >> frame2;
      auto raw_frame_dpts = frame1.pts_delta(frame2);

      srp::ffmpeg_io_container::ffmpeg_stream::stream_options options{.type =
                                                                          srp::ffmpeg_io_container::ffmpeg_stream::stream_options::stream_type::audio,
                                                                      .bitrate = copt.bitrate(),
                                                                      .pts_step = static_cast<size_t>(raw_frame_dpts),
                                                                      .audio_opt = {.sample_rate = static_cast<int>(copt.sampling_rate())}};
      output.create_stream(options);

      timestamps << "#Frame\tTimestamp[sec]\tPointId" << std::endl;
    }
    ffmpeg_reader input;
    ffmpeg_writer output;
    std::ofstream timestamps;
    std::atomic<bool> is_rec{false};
    timestamp current_frame;
    std::future<recording_info> record_thread;
  };

  std::pair<bool, std::string> make_check();
  std::pair<std::string, std::string> start_recording(std::filesystem::path const &path_template, std::string const& dev_id);
  std::pair<size_t, std::chrono::duration<double>> stop_recording();
  std::tuple<size_t, std::chrono::duration<double>, double> sync(size_t sync_id);
  std::optional<timestamp_entry> recording_state() const;

  auto name() const {
    return options_.device() + " via " + options_.iformat();
  }

private:
  static recording_info capture_function(std::unique_ptr<io_block> &io) {
    recording_info info{.frames = 0};
    LOGD << "Start recording on device " << io->input.source().name;

    auto capture_start_time = std::chrono::high_resolution_clock::now();

    auto frame_time = capture_start_time;

    native_audio_frame frame;
    while (io->is_rec) {
      io->input >> frame;

      auto current_frame_time = std::chrono::high_resolution_clock::now();
      timestamp_entry ts{.frame_id = info.frames,
                         .ts = current_frame_time - capture_start_time,
                         .fps = 1.0 / std::chrono::duration_cast<std::chrono::duration<double>>(current_frame_time - frame_time).count()};
      io->current_frame.update(ts);

      frame_time = current_frame_time;

      io->output << frame;
      ++info.frames;
    }

    auto capture_stop_time = std::chrono::high_resolution_clock::now();
    info.duration = std::chrono::duration_cast<std::chrono::duration<double>>(capture_stop_time - capture_start_time);

    LOGD << "Stop recording on device " << io->input.source().name << ". Captured " << info.frames;
    return info;
  }

  AudioCaptureOptions options_;

  std::unique_ptr<io_block> io_;
};

std::optional<audio_instance::audio_io::timestamp_entry> audio_instance::audio_io::recording_state() const {
  if (io_) {
    return io_->current_frame.ts();
  } else {
    return {};
  }
}
std::tuple<size_t, std::chrono::duration<double>, double> audio_instance::audio_io::sync(size_t sync_id) {
  if (io_) {
    auto current_frame = io_->current_frame.ts();
    io_->timestamps << current_frame.frame_id << "\t" << current_frame.ts.count() << "\t" << sync_id << std::endl;

    return {current_frame.frame_id, current_frame.ts, current_frame.fps};
  } else {
    return {0, {}, -1.0};
  }
}
std::pair<size_t, std::chrono::duration<double>> audio_instance::audio_io::stop_recording() {
  if (io_) {
    io_->is_rec = false;
    try {
      auto rec_result = io_->record_thread.get();

      io_.reset(nullptr);

      return {rec_result.frames, rec_result.duration};
    } catch (std::exception &e) {
      LOGE << "Error during recordion. Reason: " << e.what();

      io_.reset(nullptr);
      return {0, {}};
    }
  } else {
    return {1, {}};
  }
}
std::pair<std::string, std::string> audio_instance::audio_io::start_recording(std::filesystem::path const &path_template, std::string const& dev_id) {
  auto [output_path, stamp_path] = srp::PathUtils::create_file_path(options_.root(), path_template, dev_id, options_.filetype());

  io_ = std::make_unique<io_block>(options_.iformat(), options_.device(), output_path.generic_string(),
                                   stamp_path.generic_string(), options_);

  io_->record_thread = std::async(std::launch::async, [this]() {
    io_->is_rec = true;
    return capture_function(io_);
  });

  return {output_path.generic_string(), stamp_path.generic_string()};
}
std::pair<bool, std::string> audio_instance::audio_io::make_check() {
  try {
    auto check_output = "check." + options_.filetype();
    auto stamp_output = "check.stamp";

    io_ = std::make_unique<io_block>(options_.iformat(), options_.device(), check_output, stamp_output, options_);

    io_->record_thread = std::async(std::launch::async, [this]() {
      io_->is_rec = true;
      return capture_function(io_);
    });

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);

    io_->is_rec = false;

    auto check_info = io_->record_thread.get();

    io_.reset(nullptr);

    std::ostringstream oss;
    oss << "Perform check for device " << options_.device() << " [captured via " << options_.iformat() << "].\n";
    oss << "Capture " << check_info.frames << " frames [" << check_info.duration.count() << " seconds] at " << options_.sampling_rate() << " smpl/s\n";
    oss << "Stored to " << check_output << " and " << stamp_output << std::endl;

    LOGD << oss.str();
    return {true, oss.str()};
  } catch (std::exception &e) {
    io_.reset(nullptr);
    return {false, "Catch critical exception. Message: " + std::string(e.what())};
  }
}


void srp::audio_instance::init(size_t uid) {
  uid_ = uid;
  LOGD << "Receive uid " << this->uid();
}
std::optional<ClientCheckResponse> srp::audio_instance::check() {
  auto [state, info] = device_->make_check();
  ClientCheckResponse resp;
  resp.set_check_ok(state);
  resp.set_info(info);
  resp.set_type("Ffmpeg audio");
  resp.set_name(device_->name());
  return resp;
}
std::optional<ClientStartRecordResponse> srp::audio_instance::start_recording(const std::string &path_template) {
  ClientStartRecordResponse start_response;

  auto [output, stamp] = device_->start_recording(path_template, std::to_string(uid_));

  start_response.add_data_path(output);
  start_response.add_sync_point_path(stamp);

  return {start_response};
}
std::optional<ClientStopRecordResponse> srp::audio_instance::stop_recording() {
  ClientStopRecordResponse stop_resp;

  auto [frames, duration] = device_->stop_recording();

  if (frames == 0 && duration == std::chrono::duration<double>{}) return {};

  stop_resp.set_frames(frames);
  stop_resp.set_duration_sec(duration.count());
  stop_resp.set_average_fps(frames / duration.count());

  return stop_resp;
}
std::optional<ClientSyncResponse> srp::audio_instance::sync_time(size_t sync_point) {
  ClientSyncResponse sync_r;
  auto [frame, duration, fps] = device_->sync(sync_point);

  if (fps < 0) { return {}; }

  sync_r.set_sync_point(sync_point);
  sync_r.set_average_fps(fps);
  sync_r.set_frames(frame);
  sync_r.set_duration_sec(duration.count());

  return sync_r;
}
std::optional<ClientTimeResponse> srp::audio_instance::state() {
  ClientTimeResponse ctr;
  auto state = device_->recording_state();
  if (state) {
    ctr.set_state(ClientTimeResponse_CaptureState_recording);
    ctr.set_frames(state->frame_id);
    ctr.set_average_fps(state->fps);
    ctr.set_duration_sec(state->ts.count());
  } else {
    ctr.set_state(ClientTimeResponse_CaptureState_stopped);
  }
  return ctr;
}

srp::audio_instance::audio_instance(AudioCaptureOptions opt) { device_ = std::make_unique<audio_io>(std::move(opt)); }

srp::audio_instance::~audio_instance() = default;
