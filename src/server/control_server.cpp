//
// Created by yalavrinenko on 30.11.2020.
//

#include "../utils/logger.hpp"
#include "control_server.hpp"
#include "server_acceptor.hpp"
#include <chrono>
#include <utility>


namespace srp {
  using boost::asio::ip::tcp;

  class control_server::control_server_impl {
  public:
    explicit control_server_impl(control_server::connection_point endpoint) : local_endpoint_{std::move(endpoint)} {}

    void start();

    void stop();

    auto& acceptor() { return acceptor_; }

    ~control_server_impl();

  private:
    control_server::connection_point const local_endpoint_;
    std::unique_ptr<server_acceptor> acceptor_ = nullptr;

    std::future<void> acceptor_thread_;
    boost::asio::io_service io_service_;
  };

  void control_server::control_server_impl::start() {
    LOGW << "Start acceptor. ";
    try {
      if (local_endpoint_.host.empty()) acceptor_ = std::make_unique<server_acceptor>(io_service_, tcp::endpoint(tcp::v4(), local_endpoint_.port));
      else
        acceptor_ =
            std::make_unique<server_acceptor>(io_service_, tcp::endpoint(boost::asio::ip::make_address(local_endpoint_.host), local_endpoint_.port));
    } catch (boost::system::system_error const &error) {
      LOGE << "Unable to start client acceptor. Reasone:" << error.what();
      throw acceptor_create_error(error.what());
    }

    auto acceptor_launch_function = [this]() {
      acceptor_->start();
      io_service_.run();
    };

    acceptor_thread_ = std::async(std::launch::async, acceptor_launch_function);

    acceptor_thread_.wait();
  }
  void control_server::control_server_impl::stop() {
    if (acceptor_) {
      LOGW << "Try to stop acceptor....";

      acceptor_->stop();
      while (!io_service_.stopped())
        io_service_.stop();

      LOGW << "Stopped!";
      acceptor_ = nullptr;
    }
  }
  control_server::control_server_impl::~control_server_impl() { this->stop(); }

  control_server::control_server(const control_server::connection_point &endpoint): pimpl_{std::make_unique<control_server_impl>(endpoint)} {
  }

  void control_server::start() {
    if (pimpl_)
      pimpl_->start();
  }

  void control_server::stop() {
    if (pimpl_)
      pimpl_->stop();
  }

  control_server::~control_server() {
    if (pimpl_) {
      pimpl_.reset(nullptr);
    }
  }
  control_server::control_server(control_server &&rhs) noexcept {
    pimpl_ = std::move(rhs.pimpl_);
  }

  void control_server::register_session_acceptor(SessionType type, session_builder build_callback) {
    if (pimpl_->acceptor()){
      pimpl_->acceptor()->register_session_acceptor(type, std::move(build_callback));
    }
  }
}// namespace srp
