//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_CAPTURE_HPP
#define SRP_CAPTURE_HPP
#include <memory>
#include <optional>
#include <session.pb.h>
namespace srp {
  class capture_i {
  public:
    virtual void init(size_t uid) = 0;

    virtual std::optional<ClientResponse> check() = 0;

    virtual std::optional<ClientResponse> start_recording(std::string const &path_template) = 0;
    virtual std::optional<ClientResponse> stop_recording() = 0;

    virtual std::optional<ClientResponse> sync_time(size_t sync_point) = 0;

    auto uid() const { return uid_; }

  protected:
    size_t uid_;
  };

  using capture_device = std::unique_ptr<capture_i>;
}
#endif//SRP_CAPTURE_HPP
