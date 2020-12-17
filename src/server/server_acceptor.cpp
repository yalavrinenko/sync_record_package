//
// Created by yalavrinenko on 11.12.2020.
//
#include "server_acceptor.hpp"
#include "../utils/logger.hpp"

namespace srp{
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
    auto session = base_session::create_session(this->io_service_);

    auto accept_function = [this, raw_session = session](boost::system::error_code ecode) {
      if (!ecode) {
        process_connection(std::move(raw_session));
      } else if (ecode) {
        LOGW << "Fail to accept connection. Reason: " << ecode.message();
      }

      if (is_active_) accept_connection();
    };

    if (is_active_)
      acceptor_.async_accept(session->socket(), accept_function);
  }

  void server_acceptor::process_connection(std::shared_ptr<base_session> raw_session) {
    auto session_ptr = base_session::create_session(std::move(raw_session->socket()));
    raw_session.reset();
    auto client_type = session_ptr->get_type();
    LOGD << "Accept connection from " << session_ptr->remote_address() << " client type " << static_cast<int>(client_type);
    if (builder_callbacks_.contains(client_type))
      std::invoke(builder_callbacks_[client_type], std::move(session_ptr));
    else
      LOGW << "Unknown client type: " << SessionInfo::session_type_str(client_type);
  }
}