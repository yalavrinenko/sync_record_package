//
// Created by yalavrinenko on 26.01.2021.
//

#ifndef SRP_BITALINO_INSTANCE_HPP
#define SRP_BITALINO_INSTANCE_HPP

#include <capture_interface/capture.hpp>
#include <session.pb.h>
#include <options.pb.h>


namespace srp {
  class bitalino_instance: public capture_i {
  public:
    explicit bitalino_instance(BitalinoCaptureOptions opt);
    ~bitalino_instance() override;

    void init(size_t uid) override;
    std::optional<ClientCheckResponse> check() override;
    std::optional<ClientStartRecordResponse> start_recording(const std::string &path_template) override;
    std::optional<ClientStopRecordResponse> stop_recording() override;
    std::optional<ClientSyncResponse> sync_time(size_t sync_point) override;
    std::optional<ClientTimeResponse> state() override;
  private:
    class bitalino_io;
    std::unique_ptr<bitalino_io> device_;
  };
}


#endif//SRP_BITALINO_INSTANCE_HPP
