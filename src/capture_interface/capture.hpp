//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_CAPTURE_HPP
#define SRP_CAPTURE_HPP
namespace srp {
  class capture_i {
  public:
    virtual void init(size_t uid) = 0;

    virtual ClientCheckResponse check() = 0;

    virtual ClientStartRecordResponse start_recording(std::string const &path_template) = 0;
    virtual ClientStopRecordResponse stop_recording() = 0;

    virtual ClientSyncResponse sync_time(size_t sync_point) = 0;
  };
}
#endif//SRP_CAPTURE_HPP
