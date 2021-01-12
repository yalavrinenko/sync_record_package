//
// Created by yalavrinenko on 11.12.2020.
//

#ifndef SRP_SERVER_ACCEPTOR_HPP
#define SRP_SERVER_ACCEPTOR_HPP
#include "net/sessions.hpp"
#include <boost/asio.hpp>
#include <functional>

namespace srp {
  class acceptor_create_error : public std::exception {
  public:
    explicit acceptor_create_error(std::string_view reason) : what_{std::move(reason)} {}

  private:
    std::string_view what_;
  };

  class server_acceptor {
  public:
    server_acceptor(boost::asio::io_service &io_service, const tcp::endpoint &end_point)
        : io_service_{io_service}, acceptor_{io_service, end_point}, is_active_{true} {
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
    void process_connection(std::shared_ptr<base_session> session);

    std::string info() const {
      using namespace std::string_literals;
      return acceptor_.local_endpoint().address().to_string() + " port ["s + std::to_string(acceptor_.local_endpoint().port()) + "]"s;
    }

  private:
    void accept_connection();

    boost::asio::io_service &io_service_;

    tcp::acceptor acceptor_;
    std::atomic<bool> is_active_;
    std::unordered_map<SessionType, session_builder> builder_callbacks_;
  };
}


#endif//SRP_SERVER_ACCEPTOR_HPP
