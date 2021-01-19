//
// Created by yalavrinenko on 11.12.2020.
//
#include "server_acceptor.hpp"
#include "../utils/logger.hpp"

#include "net/netcomm.hpp"

namespace srp {
  void server_acceptor::register_session_acceptor(SessionType type, srp::session_builder build_callback) {
    builder_callbacks_[type] = std::move(build_callback);
    LOGD << "Register builder callback for type " << SessionInfo::session_type_str(type) << ". Callback ptr "
         << builder_callbacks_[type].target_type().name();
  }

  void server_acceptor::start() {
    LOGD << "Accept connection at " << info();
    accept_connection();
  }

  void server_acceptor::stop() {
    is_active_ = false;
    auto info_copy = info();
    boost::system::error_code ecode;
    acceptor_.cancel();
    acceptor_.close(ecode);
    //acceptor_.release();
    LOGD << "Stop accepting client at " << info_copy << ": " << ecode.message();
  }

  void server_acceptor::accept_connection() {
    struct socket_handler {
      explicit socket_handler(boost::asio::io_service &ios): socket(ios){
      }
      boost::asio::ip::tcp::socket socket;
    };
    auto handler = std::make_shared<socket_handler>(this->io_service_);

    auto accept_function = [this, handler](boost::system::error_code ecode) {
      if (!ecode) {
        auto comm = std::make_unique<netcomm>(std::move(handler->socket));
        process_connection(base_session::create_session(std::move(comm)));
      } else if (ecode) {
        LOGW << "Fail to accept connection. Reason: " << ecode.message();
      }

      if (is_active_) accept_connection();
    };

    if (is_active_) acceptor_.async_accept(handler->socket, accept_function);
  }

  void server_acceptor::process_connection(std::shared_ptr<base_session> session_ptr) {
    auto client_type = session_ptr->get_type();
    LOGD << "Accept connection from " << session_ptr->remote_address() << " client type " << static_cast<int>(client_type);
    if (builder_callbacks_.contains(client_type))
      std::invoke(builder_callbacks_[client_type], std::move(session_ptr));
    else
      LOGW << "Unknown client type: " << SessionInfo::session_type_str(client_type);
  }
}// namespace srp