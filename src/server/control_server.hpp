//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_CONTROL_SERVER_HPP
#define SRP_CONTROL_SERVER_HPP
#include "iserver.hpp"
#include <future>
#include <string>
#include <memory>

namespace srp {
  class control_server : public iserver {
  public:
    struct connection_point {
      std::string host;
      unsigned int port;
    };

    explicit control_server(const connection_point &endpoint);
    control_server(control_server const& rhs) = delete;
    control_server(control_server&& rhs) noexcept;

    void start() override;

    void stop() override;

    ~control_server();

  private:
    class control_server_impl;
    std::unique_ptr<control_server_impl> pimpl_;
  };
}// namespace srp

#endif//SRP_CONTROL_SERVER_HPP
