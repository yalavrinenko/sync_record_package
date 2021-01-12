//
// Created by yalavrinenko on 17.12.2020.
//
#include "net/sessions.hpp"

#include "rec_client.hpp"
#include "../protocols/actions.hpp"

namespace srp {

  struct recording_client::recording_client_impl{
    explicit recording_client_impl(std::shared_ptr<srp::base_session> session) : session_{std::move(session)} {
    }

    template<typename response_t>
    std::optional<response_t> wait_response(){
      auto resp = session_->receive_message<ClientResponse>();
      if (resp) {
        response_t check_status;
        if (check_status.ParseFromString(resp.value().data()))
          return check_status;
        else {
          LOGW << "Fail to parse income check message.";
          return {};
        }
      }

      LOGW << "Fail to receive income message.";
      return {};
    }

    void send_register_message(size_t uid){
      LOGD << "Try to register client " << uid;
      session_->send_message(ActionMessageBuilder::register_client(uid));
    }

    std::optional<ClientResponse> send_check_message(){
      session_->send_message(ActionMessageBuilder::check_client());
      return session_->receive_message<ClientResponse>();
    }

    std::optional<ClientResponse> send_start_message(std::string const &path_template) {
      session_->send_message(ActionMessageBuilder::start_message(path_template));
      return session_->receive_message<ClientResponse>();
    }

    std::optional<ClientResponse> send_stop_message(){
      session_->send_message(ActionMessageBuilder::stop_message());
      return session_->receive_message<ClientResponse>();
    }

    std::optional<ClientResponse> send_sync_signal(size_t sync_point){
      session_->send_message(ActionMessageBuilder::sync_message(sync_point));
      return session_->receive_message<ClientResponse>();
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
    uid_ = uid;
    pimpl_->send_register_message(uid);
  }

  std::optional<ClientCheckResponse> recording_client::check() {
    return pimpl_->send_check_message();
  }

  std::optional<ClientStartRecordResponse> recording_client::start_recording(std::string const &path_template) {
    return pimpl_->send_start_message(path_template);
  }
  std::optional<ClientStopRecordResponse> recording_client::stop_recording() {
    return pimpl_->send_stop_message();
  }
  std::optional<ClientSyncResponse> recording_client::sync_time(size_t sync_point) {
    return pimpl_->send_sync_signal(sync_point);
  }

  bool recording_client::is_connected() {
    return pimpl_->session_->is_active();
  }

  recording_client::~recording_client() = default;
}
