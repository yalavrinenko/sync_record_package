//
// Created by yalavrinenko on 19.01.2021.
//

#ifndef SRP_REMOTE_CONTROL_CLIENT_HPP
#define SRP_REMOTE_CONTROL_CLIENT_HPP

#include <capture_interface/capture_control.hpp>

namespace  srp {
  class base_session;

  class remote_control_client : public controller_i  {
  public:
    static std::unique_ptr<remote_control_client> from_base_session(std::shared_ptr<srp::base_session> const& session);

    ~remote_control_client() override;

    void start(const controller_callbacks &callbacks) override;
    void stop() override;
    void send_message(const ClientActionMessage &crp) override;

  protected:
    remote_control_client(std::shared_ptr<srp::base_session> const& session);

  private:
    class net_control_impl;

    std::unique_ptr<net_control_impl> pimpl_;
  };
}


#endif//SRP_REMOTE_CONTROL_CLIENT_HPP
