//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_AUDIO_INSTANCE_HPP
#define SRP_AUDIO_INSTANCE_HPP

#include <capture_interface/capture.hpp>
#include <session.pb.h>
#include <options.pb.h>

namespace srp {
  class audio_instance: public capture_i {
  public:
    explicit audio_instance(AudioCaptureOptions opt);

    void init(size_t uid) override;

    std::optional<ClientCheckResponse> check() override;

    std::optional<ClientStartRecordResponse> start_recording(const std::string &path_template) override;

    std::optional<ClientStopRecordResponse> stop_recording() override;

    std::optional<ClientSyncResponse> sync_time(size_t sync_point) override;

    std::optional<ClientTimeResponse> state() override;

    ~audio_instance() override;

  private:
    class audio_io;
    std::unique_ptr<audio_io> device_;
  };
}


#endif//SRP_AUDIO_INSTANCE_HPP
