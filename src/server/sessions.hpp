//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_SESSIONS_HPP
#define SRP_SESSIONS_HPP

#include "../capture_interface/communication_i.hpp"
#include "../utils/io.hpp"
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
    enum class connection_state{
      active,
      closed
    };

    static auto create_session(std::unique_ptr<communication_i> comm) {
      class constructor: public base_session{
      public:
        explicit constructor(std::unique_ptr<communication_i> comm): base_session(std::move(comm)){}
      };
      return std::make_shared<constructor>(std::move(comm));
    }

    auto remote_address(){
      return comm_->commutator_info();
    }

    SessionType get_type();
    std::optional<SessionType> const& get_type() const;

    connection_state state() const { return state_; }
    bool is_active() const { return state() == connection_state::active; }

    auto & commutator() { return comm_; }
    auto const& commutator() const { return comm_; }

    template<typename message_t>
    bool send_message(message_t const& message) {
      bool io_result = false;
      if (is_active())
        io_result = srp::NetUtils::sync_send_proto(commutator(), message);

      if (!io_result)
        switch_state(connection_state::closed);

      return io_result;
    }

    template<typename message_t>
    std::optional<message_t> receive_message(){
      if (is_active()){
        auto message = srp::NetUtils::sync_read_proto<message_t>(commutator());
        if (!message)
          switch_state(connection_state::closed);

        return message;
      }
      return {};
    }

    ~base_session();

  protected:
    explicit base_session(std::unique_ptr<communication_i> comm) : comm_{std::move(comm)} {
      if (comm_)
        LOGD << "Create base session with " << comm_->commutator_info();
      else
        LOGW << "Unable to open stream for incoming client.";
      comm_ ? switch_state(connection_state::active) : switch_state(connection_state::closed);
    }

    private:
    void fetch_type();
    void switch_state(connection_state new_state) {
      state_ = new_state;
    }

    connection_state state_;
    std::optional<SessionType> type_{};
    std::unique_ptr<communication_i> comm_;
  };

  using session_builder = std::function<void(std::shared_ptr<base_session> raw_session)>;
}

#endif //SRP_SESSIONS_HPP
