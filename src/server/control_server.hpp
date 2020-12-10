//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_CONTROL_SERVER_HPP
#define SRP_CONTROL_SERVER_HPP
#include "../utils/logger.hpp"
#include "iserver.hpp"
#include "sessions.hpp"
#include <boost/asio.hpp>
#include <future>
#include <string>

namespace srp {

  using boost::asio::ip::tcp;

  class acceptor_create_error : public std::exception {
  public:
    explicit acceptor_create_error(std::string_view reason) : what_{std::move(reason)} {}

  private:
    std::string_view what_;
  };

  class server_acceptor {
  public:
    using session_builder = std::function<void(std::shared_ptr<base_session> raw_session)>;

    server_acceptor(boost::asio::io_service &io_service, const tcp::endpoint &end_point)
        : acceptor_{io_service, end_point}, socket_{io_service}, is_active_{true} {
      if (acceptor_.is_open()) LOGD << "Create server acceptor at " << info();
      else {
        LOGE << "Unable to start client acceptor at " << info();
        throw acceptor_create_error("Unable to start client acceptor");
      }
    }

    void register_session_acceptor(SessionType type, session_builder build_callback);

    void start();

    void stop();

  protected:
    void process_connection();

    std::string info() const {
      using namespace std::string_literals;
      return acceptor_.local_endpoint().address().to_string() + " port ["s + std::to_string(acceptor_.local_endpoint().port()) + "]"s;
    }

  private:
    void accept_connection();

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    bool is_active_;
    std::unordered_map<SessionType, session_builder> builder_callbacks_;
  };

  class control_server : public iserver {
  public:
    struct connection_point {
      std::string host;
      unsigned int port;
    };
    explicit control_server(const connection_point &endpoint) : local_endpoint_{endpoint} {}

    void start() override;

    void stop() override;

    ~control_server();

  private:
    connection_point const &local_endpoint_;
    std::unique_ptr<server_acceptor> acceptor_ = nullptr;

    std::future<void> acceptor_thread_;
    boost::asio::io_service io_service_;
  };
}// namespace srp

#endif//SRP_CONTROL_SERVER_HPP
