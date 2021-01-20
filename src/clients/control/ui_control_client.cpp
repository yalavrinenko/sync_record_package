//
// Created by yalavrinenko on 20.01.2021.
//

#include "ui_control_client.hpp"

#include <session.pb.h>
#include <options.pb.h>
#include <capture_interface/capture_control.hpp>
#include <net/sessions.hpp>
#include <net/netcomm.hpp>
#include <boost/asio.hpp>

namespace {
  boost::asio::io_service boost_ios;
}

srp::ui_control_client::ui_control_client(const srp::UiControlOption &options) {
  open_connection(options.server().host(), options.server().port());

  auto session_type = ClientWelcomeMessage();
  if (options.role() == options.monitor){
    session_type.set_type(ui);
  } else if (options.role() == options.master){
    session_type.set_type(master);
    this->can_send_messages_ = true;
  }

  session_->send_message(session_type);
}

void srp::ui_control_client::open_connection(const std::string &host, int port) {
  using namespace boost::asio::ip;
  tcp::socket socket{::boost_ios};
  socket.connect(tcp::endpoint(address::from_string(host), port));

  auto comm = std::make_unique<netcomm>(std::move(socket));
  session_ = base_session::create_session(std::move(comm));
}

void srp::ui_control_client::start(const srp::controller_callbacks &callbacks) {
  accept_log_ = true;

  auto accept_log_function = [this, callbacks](){
    LOGD << "Start receiving log " << session_->remote_address();
    while (accept_log_ && session_->is_active()){
      auto log = session_->receive_message<ClientActionMessage>(true);
      if (log) {
        auto response = srp::ProtoUtils::message_from_bytes<ClientResponse>(log->meta());
        std::invoke(callbacks.callback_map().at(response.trigger_action()), std::cref(response));
      }
    }
    LOGD << "Stop receiving log.";
  };

  log_thread_ = std::async(std::launch::async, accept_log_function);
}

void srp::ui_control_client::stop() {
  accept_log_ = false;
  log_thread_.get();
}

void srp::ui_control_client::send_message(const srp::ClientActionMessage &crp) {
  if (!can_send_messages_){
    LOGE << "Instance has monitor role. Unable to send control message.";
    return;
  }

  if (session_) {
    session_->send_message(crp);
  } else {
    LOGE << "Unable to send message. Session does not open.";
  }
}

srp::ui_control_client::~ui_control_client() = default;

const char *srp::session_closed::what() const noexcept { return "Session is closed"; }
