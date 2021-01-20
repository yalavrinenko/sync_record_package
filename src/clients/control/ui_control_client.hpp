//
// Created by yalavrinenko on 20.01.2021.
//

#ifndef SRP_UI_CONTROL_CLIENT_HPP
#define SRP_UI_CONTROL_CLIENT_HPP

#include <string>
#include <memory>
#include <future>
#include <capture_interface/capture_control.hpp>

namespace srp {
  class UiControlOption;
  class base_session;

  class session_closed: std::exception{
  public:
    session_closed() = default;

  private:
    const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override;
  };

  class ui_control_client : public  srp::controller_i {
  public:
    explicit ui_control_client(UiControlOption const& options);

    void start(const controller_callbacks &callbacks) override;
    void stop() override;

    void send_message(const ClientActionMessage &crp) override;
    ~ui_control_client() override;

  private:
    void open_connection(std::string const &host, int port);

    std::shared_ptr<srp::base_session> session_;

    bool can_send_messages_ = false;
    std::atomic<bool> accept_log_{false};
    std::future<void> log_thread_;
  };
}


#endif//SRP_UI_CONTROL_CLIENT_HPP
