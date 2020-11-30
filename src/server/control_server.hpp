//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_CONTROL_SERVER_HPP
#define SRP_CONTROL_SERVER_HPP
#include "../utils/logger.hpp"
#include "iserver.hpp"
#include "sessions.hpp"
#include <boost/asio.hpp>
#include <string>

namespace srp{

  using boost::asio::ip::tcp;

  class server_acceptor{
  public:
    using session_builder = std::function<void(std::shared_ptr<base_session> raw_session)>;

    server_acceptor(boost::asio::io_service& io_service, const tcp::endpoint& end_point):
        acceptor_{io_service, end_point}, socket_{io_service}, is_active_{true}{
      LOGD << "Create server acceptor at " << info();
    }

    void register_session_acceptor(session_type type, session_builder build_callback);

    void start();

    void stop();

  protected:
    void process_connection();

    std::string info() const {
      using namespace std::string_literals;
      return acceptor_.local_endpoint().address().to_string() + " port ["s +
             std::to_string(acceptor_.local_endpoint().port()) + "]"s;
    }

  private:
    void accept_connection();

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    bool is_active_;
    std::unordered_map<session_type, session_builder> builder_callbacks_;
  };

  class control_server: public iserver{
  public:
    void start() override;

    void stop() override;
  };
}

#endif //SRP_CONTROL_SERVER_HPP
