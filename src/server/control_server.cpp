//
// Created by yalavrinenko on 30.11.2020.
//

#include "control_server.hpp"
#include <memory>
#include <chrono>



void srp::control_server::start() {
  LOGW << "Start acceptor. ";
  try {
    if (local_endpoint_.host.empty()) acceptor_ = std::make_unique<server_acceptor>(io_service_, tcp::endpoint(tcp::v4(), local_endpoint_.port));
    else
      acceptor_ =
          std::make_unique<server_acceptor>(io_service_, tcp::endpoint(boost::asio::ip::make_address(local_endpoint_.host), local_endpoint_.port));
  } catch (boost::system::system_error const& error){
    LOGE << "Unable to start client acceptor. Reasone:" << error.what();
    throw acceptor_create_error(error.what());
  }

  auto acceptor_launch_function = [this](){
    acceptor_->start();
    io_service_.run();
  };

  acceptor_thread_ = std::async(std::launch::async, acceptor_launch_function);

  acceptor_thread_.wait();
}

void srp::control_server::stop() {
  if (acceptor_) {
    LOGW << "Try to stop acceptor....";

    acceptor_->stop();
    io_service_.stop();

    using namespace std::chrono_literals;
    auto result = acceptor_thread_.wait_for(2s);

    if (result == std::future_status::timeout)
      LOGW << "Fail to stop acceptor. Force shutdown.";
    else
      LOGW << "Acceptor stopped.";

    acceptor_ = nullptr;
  } else {
    LOGW << "Nothing to stop.";
  }
}
srp::control_server::~control_server() {
  this->stop();
}

void srp::server_acceptor::register_session_acceptor(SessionType type,
                                                     srp::server_acceptor::session_builder build_callback) {
  builder_callbacks_[type] = std::move(build_callback);
  LOGD << "Register builder callback for type " << SessionInfo::session_type_str(type) << ". Callback ptr "
       << builder_callbacks_[type].target_type().name();
}

void srp::server_acceptor::start() {
  LOGD << "Accept connection at " << info();
  accept_connection();
}

void srp::server_acceptor::stop() {
  is_active_ = false;
  auto info_copy = info();
  acceptor_.release();
  LOGD << "Stop accepting client at " << info_copy;
}

void srp::server_acceptor::accept_connection() {
  auto accept_function = [this](boost::system::error_code ecode) {
    if (!ecode) {
      process_connection();
    } else if (ecode != boost::asio::error::operation_aborted){
      LOGW << "Fail to accept connection. Reason: " << ecode.message();
    }

    if (ecode != boost::asio::error::operation_aborted)
      accept_connection();
  };

  acceptor_.async_accept(socket_, accept_function);
}

void srp::server_acceptor::process_connection() {
  auto session_ptr = base_session::create_session(std::move(socket_));
  auto client_type = session_ptr->get_type();
  LOGD << "Accept connection from " << session_ptr->remote_address() << " client type " << static_cast<int>(client_type);
  if (builder_callbacks_.contains(client_type))
    std::invoke(builder_callbacks_[client_type], std::move(session_ptr));
  else
    LOGW << "Unknown client type: " << SessionInfo::session_type_str(client_type);
}
