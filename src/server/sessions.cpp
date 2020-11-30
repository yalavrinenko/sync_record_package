//
// Created by yalavrinenko on 30.11.2020.
//

#include "sessions.hpp"

srp::session_type srp::base_session::get_type() {
  uint8_t type;
  boost::system::error_code ecode;
  boost::asio::read(socket_, boost::asio::buffer(&type, sizeof(type)), ecode);

  if (!ecode){
    LOGW << "Unable to read connection type from " << socket_.remote_endpoint().address().to_string() << ". Reason: "
         << ecode.message();
    return session_type::undefined;
  }

  return session_type_from_raw(type);
}
