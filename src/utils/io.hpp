//
// Created by yalavrinenko on 10.12.2020.
//

#ifndef SRP_IO_HPP
#define SRP_IO_HPP

#include "logger.hpp"
#include <boost/asio.hpp>
#include <google/protobuf/util/delimited_message_util.h>
#include <google/protobuf/util/json_util.h>
#include <filesystem>
#include <fstream>

namespace srp {
  class message_parse_error : public std::exception {};

  struct NetUtils {

    struct IoParameters {
      static constexpr auto AWAIT_TIMEOUT() {
        return std::chrono::milliseconds(20000);
      }
    };

    template<typename buffer_t, typename stream_t>
    static auto sync_read_message(stream_t &stream, buffer_t const &buffer) {
      if (stream >> buffer) {
        LOGW << "Unable to read from socket"
             << ". Reason: " << stream.error().message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename communication_t>
    static std::optional<proto_t> sync_read_proto(communication_t &comm, bool wait = false) {
      proto_t message;
      if (comm->is_alive()){
        if (!wait) {
          auto reader = [&]() { return comm->receive_message(&message); };

          auto waiter = std::async(std::launch::async, reader);

          auto status = waiter.wait_for(IoParameters::AWAIT_TIMEOUT());

          if (status == std::future_status::ready) {
            return waiter.get() ? message : std::optional<proto_t>{};
          } else {
            comm->terminate();
            waiter.get();
            return {};
          }
        } else {
          auto state = comm->receive_message(&message);
          return (state) ? message : std::optional<proto_t>{};
        }

      } else {
        LOGW << "Unable to read proto. Communication !is_alive.";
      }

      return {};
    }

    template<typename buffer_t, typename stream_t>
    static auto sync_send_message(stream_t &stream, buffer_t const &buffer) {
      stream << buffer; stream.flush();
      if (!stream) {
        LOGW << "Fail to write to socket"
             << ". Reason: " << stream.error().message();
        return false;
      }

      return true;
    }

    template<typename proto_t, typename communication_t>
    static bool sync_send_proto(communication_t &comm, proto_t const &data) {
      if (comm->is_alive()){
        return comm->send_message(&data);
      } else {
        LOGW << "Unable to send proto. Communication !is_alive";
      }
      return false;
    }
  };

  struct ProtoUtils {
    template<typename message_t>
    static message_t message_from_bytes(auto const &data) {
      message_t m;
      if (m.ParseFromArray(data.data(), data.size())) return m;
      else
        throw message_parse_error();
    }

    template <typename message_t>
    static std::optional<message_t> message_from_json(auto const& json){
      message_t entry;
      auto status = google::protobuf::util::JsonStringToMessage(json, &entry);

      if (!status.ok()) {
        LOGE << "Fail to parse input json. Reason: " << status.message();
        return {};
      }
      return entry;
    }
  };

  struct IoUtils{
    static std::string read_json(std::filesystem::path const &path) {
      std::ifstream in(path);

      std::string json;
      std::string line;
      while (std::getline(in, line)) { json += line + "\n"; }
      return json;
    }

    template<typename timestamp_entry>
    struct tsafe_timestamp {
    public:
      [[nodiscard]] auto const &ts() const {
        std::lock_guard lg(lock_);
        return entry_;
      }

      void update(timestamp_entry const &rhs) {
        std::lock_guard lg(lock_);
        entry_ = rhs;
      }

    private:
      mutable std::mutex lock_;
      timestamp_entry entry_{};
    };
  };

  struct TimeDateUtils {
    static std::string time_point_to_string(std::chrono::system_clock::time_point const &tp) {
      using namespace std;
      using namespace std::chrono;

      auto ttime_t = system_clock::to_time_t(tp);
      auto tp_sec = system_clock::from_time_t(ttime_t);
      milliseconds ms = duration_cast<milliseconds>(tp - tp_sec);

      std::tm *ttm = localtime(&ttime_t);

      char const *date_time_format = "%Y-%m-%d_%H:%M:%S";

      char time_str[64];

      strftime(time_str, sizeof(time_str), date_time_format, ttm);

      string result(time_str);
      return result;
    }

    static auto duration_to_str(auto const &duration) {
      auto in_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

      char time_str[256];

      auto millis = in_millis % 1000;
      in_millis /= 1000;
      auto hh = (in_millis / (60 * 60));
      in_millis %= 60 * 60;
      auto mm = in_millis / 60;
      in_millis %= 60;
      auto ss = in_millis;

      std::sprintf(time_str, "%02lld:%02lld:%02lld.%03lld", static_cast<long long int>(hh), static_cast<long long int>(mm),
                   static_cast<long long int>(ss), static_cast<long long int>(millis));
      return std::string{time_str};
    }
  };
};// namespace srp

#endif//SRP_IO_HPP
