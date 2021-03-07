//
// Created by yalavrinenko on 07.03.2021.
//

#include "pupil_eyetracker_instance.hpp"
#include "pupil_eye_io.hpp"
#include <boost/format.hpp>
#include <fstream>
#include <thread>
#include <utils/logger.hpp>
#include <future>
#include <string>
#include <filesystem>
#include <utils/io.hpp>

namespace srp {

  namespace {
    void create_external_process(std::string const &command, std::string const &wdir) {
      auto command_str = boost::format("cd %1%; %2% &") % wdir % command;
      LOGD << "Try to execute command: " << command_str;
      std::system(command_str.str().c_str());
    }
  }// namespace

  class pupil_eyetracker_instance::instance_implementation {
  public:
    instance_implementation(std::string host, long port): host_{std::move(host)}, port_{port}{}

    auto start(std::filesystem::path const& record_dir, std::filesystem::path const& stamps);
    auto stop();

    auto write_sync_point(size_t id);

    std::tuple<bool, std::chrono::milliseconds, std::string> check_device();
    [[nodiscard]] auto is_recording() const {
      return is_recording_;
    }

    auto recording_time() {
      if (io_)
        return io_->device.timestamp();
      else {
        return std::chrono::milliseconds {0};
      }
    }
  private:

    void init_device(std::filesystem::path const& stamp_path) {
      if (io_ == nullptr) {
        io_ = std::make_unique<io_block>(host_, port_, stamp_path);
      }
    }

    void reset_device() {
      io_.reset(nullptr);
    }

    std::string host_;
    long port_;

    struct io_block{
      pupil_eye_io device;
      std::ofstream stamps;
      io_block(std::string const &host, long port, std::filesystem::path const& spath): device(host, port), stamps(spath){
        stamps << "#Timestamp[millis]\tPointId" << std::endl;
      }
    };
    std::unique_ptr<io_block> io_ = nullptr;

    bool is_recording_ = false;
  };

  auto pupil_eyetracker_instance::instance_implementation::start(std::filesystem::path const &record_dir, std::filesystem::path const &stamps) {
    if (!io_)
      init_device(stamps);

    LOGD << "Start recording eyetracker data to " << record_dir << " with stamp file " << stamps;

    is_recording_ = true;
    return io_->device.start_recording(record_dir);
  }
  auto pupil_eyetracker_instance::instance_implementation::stop() {
    if (io_) {
      auto recorded_millis = io_->device.stop_recording();
      LOGD << "Stop recording eyetracker data. Saved " << recorded_millis.count() << " millis.";
      reset_device();
      is_recording_ = false;
      return recorded_millis;
    } else
      return std::chrono::milliseconds { };
  }

  std::tuple<bool, std::chrono::milliseconds, std::string> pupil_eyetracker_instance::instance_implementation::check_device() {
    try {
      start("check", "check.stamp");
      auto version = io_->device.name();

      std::this_thread::sleep_for(std::chrono::seconds(1));
      auto recorded_millis = stop();

      return {true, recorded_millis, version};
    } catch (std::exception const& e){
      LOGE << boost::format ("Critical error during device check. Reason: %1%") % e.what();
      return {false, std::chrono::milliseconds {}, e.what()};
    }
  }

  auto pupil_eyetracker_instance::instance_implementation::write_sync_point(size_t id) {
    auto time = io_->device.timestamp();
    io_->stamps << time.count() << "\t" << id << std::endl;
    return time;
  }

  void srp::pupil_eyetracker_instance::init(size_t uid) { uid_ = uid; }

  std::optional<ClientCheckResponse> srp::pupil_eyetracker_instance::check() {
    auto [state, rec, addition_message] = impl_->check_device();

    ClientCheckResponse srp;
    srp.set_check_ok(state);

    if (state) { srp.set_info((boost::format("Check capture from eye tracker. Recorded %1% millis.") % rec.count()).str()); }
    else { srp.set_info("Fail to check eye tracker. Reason: " + addition_message); }

    srp.set_type("Pupil-lab software");
    srp.set_name(((state) ? addition_message : "undefined"));

    return srp;
  }


  std::optional<ClientStartRecordResponse> srp::pupil_eyetracker_instance::start_recording(const std::string &path_template) {
    ClientStartRecordResponse rec_resp;

    auto timepoint = srp::TimeDateUtils::time_point_to_string(std::chrono::system_clock::now());
    using namespace std::string_literals;

    auto basepath = std::filesystem::path(option_.root()) / path_template;
    basepath += "."s + timepoint;
    auto output_path = basepath.string();
    auto stamp_path = basepath.string() + ".stamp";

    auto state = impl_->start(output_path, stamp_path);

    rec_resp.add_data_path(output_path);
    rec_resp.add_sync_point_path(stamp_path);

    return rec_resp;
  }

  std::optional<ClientStopRecordResponse> srp::pupil_eyetracker_instance::stop_recording() {
    ClientStopRecordResponse resp;
    auto record_time = impl_->stop();
    resp.set_duration_sec(record_time.count() / 1000.0);
    return resp;
  }

  std::optional<ClientSyncResponse> srp::pupil_eyetracker_instance::sync_time(size_t sync_point) {
    auto ts = impl_->write_sync_point(sync_point);
    ClientSyncResponse r;
    r.set_sync_point(sync_point);
    r.set_duration_sec(ts.count() / 1000.0);

    return r;
  }
  std::optional<ClientTimeResponse> srp::pupil_eyetracker_instance::state() {
    auto ts = impl_->recording_time();
    ClientTimeResponse r;
    r.set_duration_sec(ts.count() / 1000.0);
    r.set_state((impl_->is_recording()) ? ClientTimeResponse_CaptureState::ClientTimeResponse_CaptureState_recording :
                ClientTimeResponse_CaptureState_stopped);

    return r;
  }
  pupil_eyetracker_instance::~pupil_eyetracker_instance() = default;

  pupil_eyetracker_instance::pupil_eyetracker_instance(PupilEyetrackerOption option)
      : option_(std::move(option)), impl_(std::make_unique<instance_implementation>(option_.pupil_capture_host(), option_.pupil_capture_port())) {
    if (!option_.pupil_capture_bin().empty()) {
      LOGD << "Start Pupil Capture application.";
      create_external_process(option_.pupil_capture_bin() + " --port " + std::to_string(option_.pupil_capture_port()),
                              option_.pupil_capture_wdir());
    }
  }
}// namespace srp
