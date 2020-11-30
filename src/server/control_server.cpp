//
// Created by yalavrinenko on 30.11.2020.
//

#include "control_server.hpp"
#include <memory>

void srp::control_server::start() {

}

void srp::control_server::stop() {

}

void srp::server_acceptor::register_session_acceptor(srp::session_type type,
                                                     srp::server_acceptor::session_builder build_callback) {
  builder_callbacks_[type] = std::move(build_callback);
  LOGD << "Register builder callback for type " << type << ". Callback ptr " << &build_callback;
}

void srp::server_acceptor::start() {
  LOGD << "Accept connection at " << info();
  accept_connection();
}

void srp::server_acceptor::stop() {
  is_active_ = false;
  acceptor_.release();
  LOGD << "Stop accepting client at " << info();
}

void srp::server_acceptor::accept_connection() {
  auto accept_function = [this](boost::system::error_code ecode) {
    if (!ecode) {

    } else if (ecode != boost::asio::error::operation_aborted){
      LOGW << "Fail to accept connection. Reason: " << ecode.message();
    }

    if (ecode != boost::asio::error::operation_aborted)
      accept_connection();
  };

  acceptor_.async_accept(socket_, accept_function);
}

void srp::server_acceptor::process_connection() {
  auto session_ptr = std::make_shared<base_session>(std::move(socket_));
  auto client_type = session_ptr->get_type();


}
