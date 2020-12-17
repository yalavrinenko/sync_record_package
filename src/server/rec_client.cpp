//
// Created by yalavrinenko on 17.12.2020.
//
#include "sessions.hpp"

#include "rec_client.hpp"
#include "../protocols/actions.hpp"

namespace srp {

  struct recording_client::recording_client_impl{
    explicit recording_client_impl(std::shared_ptr<srp::base_session> session) : session_{std::move(session)} {
    }

    void send_register_message(size_t uid){
      session_->send_message(ActionMessageBuilder::register_client(uid));
    }

    std::shared_ptr<srp::base_session> session_;
  };

  srp::recording_client::recording_client(std::shared_ptr<srp::base_session> session) {
    pimpl_ = std::make_unique<recording_client_impl>(std::move(session));
  }

  std::unique_ptr<recording_client> srp::recording_client::from_base_session(std::shared_ptr<base_session> session) {
    struct constructor : public recording_client{
      explicit constructor(std::shared_ptr<base_session> s_ptr) : recording_client(std::move(s_ptr)) {}
    };

    return std::make_unique<constructor>(std::move(session));
  }

  void recording_client::init(size_t uid) {
    pimpl_->send_register_message(uid);
  }

  void recording_client::check() {}

  void recording_client::start_recording() {}
  void recording_client::stop_recording() {}
  void recording_client::sync_time() {}


  bool recording_client::is_connected() {
    return pimpl_->session_->is_active();
  }



  recording_client::~recording_client() = default;
}
