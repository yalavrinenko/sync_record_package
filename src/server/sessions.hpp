//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_SESSIONS_HPP
#define SRP_SESSIONS_HPP

#include <boost/asio.hpp>
#include "../utils/logger.hpp"
#include "session.pb.h"
#include "../utils/io.hpp"

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
    enum class connection_state{
      active,
      closed
    };

    static auto create_session(tcp::socket &&socket) {
      class constructor: public base_session{
      public:
        explicit constructor(tcp::socket &&socket): base_session(std::move(socket)){}
      };
      return std::make_shared<constructor>(std::move(socket));
    }

    static auto create_session(boost::asio::io_service &ios) {
      class constructor: public base_session{
      public:
        explicit constructor(boost::asio::io_service &ios): base_session(ios){}
      };
      return std::make_shared<constructor>(ios);
    }

    decltype(auto) remote_address() const {
      return socket_.remote_endpoint().address();
    }

    SessionType get_type();
    std::optional<SessionType> const& get_type() const;

    connection_state state() const { return state_; }
    bool is_active() const { return state() == connection_state::active; }

    auto & socket() { return socket_; }
    auto const& socket() const { return socket_; }

    template<typename message_t>
    bool send_message(message_t const& message) {
      bool io_result = false;
      if (is_active())
        io_result = srp::NetUtils::sync_send_proto(socket(), message);

      if (!io_result)
        switch_state(connection_state::closed);

      return io_result;
    }

    template<typename message_t>
    std::optional<message_t> receive_message(){
      if (is_active()){
        auto message = srp::NetUtils::sync_read_proto<message_t>(socket());
        if (!message)
          switch_state(connection_state::closed);

        return message;
      }
      return {};
    }


    ~base_session();

  protected:
    explicit base_session(tcp::socket &&socket) : socket_{std::move(socket)} {
      LOGD << "Create base session with " << socket_.remote_endpoint().address();
      socket_.is_open() ? switch_state(connection_state::active) : switch_state(connection_state::closed);
    }

    explicit base_session(boost::asio::io_service &ios) : socket_(ios){
      switch_state(connection_state::closed);
    }

    private:
    void fetch_type();
    void switch_state(connection_state new_state) {
      state_ = new_state;
    }

    connection_state state_;
    std::optional<SessionType> type_{};
    tcp::socket socket_;
  };

  using session_builder = std::function<void(std::shared_ptr<base_session> raw_session)>;
}

#endif //SRP_SESSIONS_HPP
