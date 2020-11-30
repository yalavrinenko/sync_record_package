//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_SESSIONS_HPP
#define SRP_SESSIONS_HPP

#include <boost/asio.hpp>
#include "../utils/logger.hpp"

namespace srp {
  using boost::asio::ip::tcp;

  enum class session_type {
    undefined = 0,
    ui = 1,
    client = 2,
  };

  session_type session_type_from_raw(auto type_id){
    switch (type_id){
      case 1: return session_type::ui;
      case 2: return session_type::client;
      default: return session_type::undefined;
    }
  }

  class base_session : public std::enable_shared_from_this<base_session> {
  public:
    explicit base_session(tcp::socket socket) : socket_{std::move(socket)} {
      LOGD << "Create base session with " << socket_.remote_endpoint().address() << std::endl;
    }

    session_type get_type();

  private:
    tcp::socket socket_;
  };
}

#endif //SRP_SESSIONS_HPP
