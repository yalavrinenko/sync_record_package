//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_SESSIONS_HPP
#define SRP_SESSIONS_HPP

#include <boost/asio.hpp>
#include "../utils/logger.hpp"
#include "session.pb.h"

namespace srp {
  using boost::asio::ip::tcp;

  class SessionInfo {
  public:
    static std::string_view session_type_str(SessionType t) {
      return SessionType_descriptor()->FindValueByNumber(t)->name();
    }
  };

  class base_session : public std::enable_shared_from_this<base_session> {
  public:
    static auto create_session(tcp::socket socket) {
      class constructor: public base_session{
      public:
        explicit constructor(tcp::socket soc): base_session(std::move(soc)){}
      };
      return std::make_shared<constructor>(std::move(socket));
    }

    decltype(auto) remote_address() const {
      return socket_.remote_endpoint().address();
    }

    SessionType get_type();
    std::optional<SessionType> const& get_type() const;

    ~base_session();

  protected:
    explicit base_session(tcp::socket socket) : socket_{std::move(socket)} {
      LOGD << "Create base session with " << socket_.remote_endpoint().address();
    }

  private:
    enum class connection_code{
      disconnect = 99
    };

    void fetch_type();

    std::optional<SessionType> type_{};
    tcp::socket socket_;
  };
}

#endif //SRP_SESSIONS_HPP
