//
// Created by yalavrinenko on 11.12.2020.
//

#ifndef SRP_SERVER_ACCEPTOR_HPP
#define SRP_SERVER_ACCEPTOR_HPP
#include <boost/asio.hpp>
#include <functional>
#include "sessions.hpp"

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
    std::atomic<bool> is_active_;
    std::unordered_map<SessionType, session_builder> builder_callbacks_;
  };
}


#endif//SRP_SERVER_ACCEPTOR_HPP
