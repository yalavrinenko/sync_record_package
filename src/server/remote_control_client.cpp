//
// Created by yalavrinenko on 19.01.2021.
//

#include "remote_control_client.hpp"
#include <net/sessions.hpp>
#include <utils/logger.hpp>
class srp::remote_control_client::net_control_impl{
public:
  explicit net_control_impl(std::shared_ptr<srp::base_session> session): session_{std::move(session)} {
  }

  void start(controller_callbacks const& callbacks);
  void stop();

  void send_message(srp::ClientActionMessage const& cam);

private:
  void accept_signal(controller_callbacks callbacks);

  std::atomic<bool> accept_signals_;
  std::shared_ptr<srp::base_session> session_;
  std::future<void> control_thread_;
};

void srp::remote_control_client::net_control_impl::accept_signal(srp::controller_callbacks callbacks) {
  LOGD << "Start receiving control signals from " << session_->remote_address();
  while (accept_signals_ && session_->is_active()){
    auto signal = session_->receive_message<ClientActionMessage>(true);
    if (signal) {
      std::invoke(callbacks.callback_map().at(signal->action()), std::cref(signal.value()));
    }
  }
  LOGD << "Stop receiving control signals.";
}
void srp::remote_control_client::net_control_impl::start(const srp::controller_callbacks &callbacks) {
  accept_signals_ = true;
  control_thread_ = std::async(std::launch::async, [this](auto const& callbacks){
    this->accept_signal(callbacks);
  }, std::cref(callbacks));
}
void srp::remote_control_client::net_control_impl::stop() {
  accept_signals_ = false;
  control_thread_.get();
}
void srp::remote_control_client::net_control_impl::send_message(const srp::ClientActionMessage &cam) {
  if (session_->is_active())
    session_->send_message(cam);
}

void srp::remote_control_client::start(const controller_callbacks &callbacks) { pimpl_->start(callbacks);
}
void srp::remote_control_client::stop() { pimpl_->stop();
}

srp::remote_control_client::remote_control_client(const std::shared_ptr<srp::base_session> &session) {
  pimpl_ = std::make_unique<net_control_impl>(session);
}
std::unique_ptr<srp::remote_control_client> srp::remote_control_client::from_base_session(const std::shared_ptr<srp::base_session> &session) {
  class dummy : public remote_control_client {
  public:
    dummy(std::shared_ptr<srp::base_session> const& session): remote_control_client(session){}
  };

  return std::make_unique<dummy>(session);
}
void srp::remote_control_client::send_message(const srp::ClientActionMessage &crp) {
  if (pimpl_)
    pimpl_->send_message(crp);
  else {
    LOGE << "Use uninitialized calss. Mine respektirung!";
  }
}

srp::remote_control_client::~remote_control_client() = default;
