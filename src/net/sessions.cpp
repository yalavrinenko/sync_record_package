//
// Created by yalavrinenko on 30.11.2020.
//

#include "sessions.hpp"
#include <protocols/actions.hpp>

srp::SessionType srp::base_session::get_type() {
  if (!type_)
    fetch_type();

  return type_.value();
}

srp::base_session::~base_session() {
  if (comm_->is_alive()){
    send_message(ActionMessageBuilder::disconnect_action());
  }
}

void srp::base_session::fetch_type() {
  type_ = SessionType::undefined;
  auto welcome = receive_message<ClientWelcomeMessage>();
  if (!welcome){
    LOGW << "Unable to read connection type.";
  } else {
    type_ = welcome.value().type();
  }
}

std::optional<srp::SessionType> const& srp::base_session::get_type() const {
  return type_;
}
