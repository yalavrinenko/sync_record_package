//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_CAPTURE_HPP
#define SRP_CAPTURE_HPP
namespace srp {
  class capture_i {
  public:
    virtual void init(size_t uid) = 0;

    virtual void check() = 0;

    virtual void start_recording() = 0;
    virtual void stop_recording() = 0;

    virtual void sync_time() = 0;
  };
}
#endif//SRP_CAPTURE_HPP
