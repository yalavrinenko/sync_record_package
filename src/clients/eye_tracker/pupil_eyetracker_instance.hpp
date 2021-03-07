//
// Created by yalavrinenko on 07.03.2021.
//

#ifndef SRP_PUPIL_EYETRACKER_INSTANCE_HPP
#define SRP_PUPIL_EYETRACKER_INSTANCE_HPP

#include <capture_interface/capture.hpp>
#include <session.pb.h>
#include <options.pb.h>

namespace srp {
  class pupil_eyetracker_instance: public capture_i {
  public:
    pupil_eyetracker_instance(PupilEyetrackerOption option);
    void init(size_t uid) override;
    std::optional<ClientCheckResponse> check() override;
    std::optional<ClientStartRecordResponse> start_recording(const std::string &path_template) override;
    std::optional<ClientStopRecordResponse> stop_recording() override;
    std::optional<ClientSyncResponse> sync_time(size_t sync_point) override;
    std::optional<ClientTimeResponse> state() override;
    ~pupil_eyetracker_instance() override;
  private:
    class instance_implementation;

    PupilEyetrackerOption option_;
    std::unique_ptr<instance_implementation> impl_;

    size_t uid_{};
  };
}


#endif//SRP_PUPIL_EYETRACKER_INSTANCE_HPP
