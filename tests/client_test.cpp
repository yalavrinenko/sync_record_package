//
// Created by yalavrinenko on 17.12.2020.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include "../src/protocols/actions.hpp"
#include "../src/server/netcomm.hpp"
#include "../src/server/rec_client.hpp"
#include "../src/server/sessions.hpp"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <google/protobuf/util/json_util.h>
#include <unordered_map>
using boost::asio::ip::tcp;
using namespace srp;
using namespace std::chrono_literals;

namespace {
  bool reply_on_message = false;
}

void delay(auto const &duration) { std::this_thread::sleep_for(duration); }

struct MessageProcess {
  static std::optional<ClientResponse> process(srp::ClientActionMessage const &message) {
    [[maybe_unused]] static auto to_json = [](auto const &message) {
      std::string str;
      auto status = google::protobuf::util::MessageToJsonString(message, &str);
      return str;
    };
    static auto default_response = [](ActionType action) {
      ClientResponse cr;
      cr.set_uid(58);
      cr.set_trigger_action(action);
      return cr;
    };
    static std::unordered_map<srp::ActionType, std::function<std::optional<ClientResponse>(srp::ClientActionMessage const &)>> const action = {
        {srp::ActionType::disconnect, [&](auto const &message) { LOGD << to_json(message); return std::optional<ClientResponse>{}; }},
        {srp::ActionType::registration, [&](auto const &message){
           auto cp = message; cp.set_meta(to_json(srp::ProtoUtils::message_from_bytes<srp::ClientRegistrationMessage>(message.meta())));
           LOGD << to_json(cp);
           return std::optional<ClientResponse>{};
         }},
        {ActionType::check_device, [&](auto const &message){
            LOGD << to_json(message);
            if (reply_on_message){
              auto cr = default_response(ActionType::check_device);
              ClientCheckResponse chr; chr.set_check_ok(true);
              chr.set_info("Check all device");
              cr.set_data(chr.SerializeAsString());
              return std::optional<ClientResponse>{cr};
            } else
              return std::optional<ClientResponse>{};
         }},
        {ActionType::start, [&](auto const &message){
            LOGD << to_json(message);
            if (reply_on_message){
              auto cr = default_response(ActionType::start);
              auto pattern = ProtoUtils::message_from_bytes<ClientStartRecord>(message.meta()).path_pattern();
              ClientStartRecordResponse csr; csr.add_data_path( pattern+ ".dat");
              csr.add_sync_point_path(pattern + ".sync");
              cr.set_data(csr.SerializeAsString());
              return std::optional<ClientResponse>{cr};
            }
            return std::optional<ClientResponse>{};
        }},
       {ActionType::stop, [&](auto const &message){
            LOGD << to_json(message);
            if (reply_on_message) {
              auto cr = default_response(ActionType::stop);
              ClientStopRecordResponse data; data.set_average_fps(50.33);
              data.set_duration_sec(128);
              data.set_frames(800);
              cr.set_data(data.SerializeAsString());
              return std::optional<ClientResponse>{cr};
            }
            return std::optional<ClientResponse>{};
          }},
        { ActionType::sync_time, [&](auto const &message){
           LOGD << to_json(message);
           if (reply_on_message){
             auto cr = default_response(sync_time);
             ClientSyncResponse data; data.set_sync_point(ProtoUtils::message_from_bytes<ClientSync>(message.meta()).sync_point());
             data.set_frames(42);
             data.set_duration_sec(18.90);
             data.set_frames(18);
             cr.set_data(data.SerializeAsString());
             return std::optional<ClientResponse>{cr};
           }
           return std::optional<ClientResponse>{};
         }},
    };

    if (action.contains(message.action())) return std::invoke(action.at(message.action()), message);
    else
      LOGE << "Unknown action!";
    return {};
  }
};

class session {
public:
  explicit session(tcp::socket socket_) : ios_(new srp::netcomm(std::move(socket_))) {}

  auto &stream() { return ios_; }

  void start() {
    sync_read_thread_ = std::async(std::launch::async, [this]() { this->sync_read(); });
  }

  [[nodiscard]] auto const &messages() const { return messages_; }

private:
  void sync_read() {
    std::optional<srp::ClientActionMessage> message;
    do {
      message = srp::NetUtils::sync_read_proto<srp::ClientActionMessage>(ios_);
      if (message) {
        messages_.emplace_back(message);
        auto res = MessageProcess::process(message.value());
        if (res) { NetUtils::sync_send_proto(ios_, res.value()); }
      }
    } while (message);
  }

  std::unique_ptr<netcomm> ios_;
  enum { max_length = 1024 };
  std::future<void> sync_read_thread_;
  std::vector<std::optional<srp::ClientActionMessage>> messages_;
};

class server {
public:
  server(boost::asio::io_service &io_service, short port) : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
  }

  auto &session_ptr() { return session_; }

private:
  void start_accept() {
    struct socket_handler {
      explicit socket_handler(boost::asio::io_service &ios) : socket(ios) {}
      boost::asio::ip::tcp::socket socket;
    };
    auto handler = std::make_shared<socket_handler>(this->io_service_);

    acceptor_.async_accept(handler->socket, [this, handler](auto v2) {
      session_ = std::make_unique<session>(std::move(handler->socket));
      this->handle_accept(session_, v2);
    });
  }

  static void handle_accept(std::unique_ptr<session> &new_session, const boost::system::error_code &error) {
    if (!error) { new_session->start(); }
  }

  std::unique_ptr<session> session_;
  boost::asio::io_service &io_service_;
  tcp::acceptor acceptor_;
};

class Client {
public:
  explicit Client(unsigned short port) : socket{ios}, endpoint{boost::asio::ip::address::from_string("127.0.0.1"), port} { socket.connect(endpoint); }

  auto &Socket() { return socket; }

  ~Client() = default;

protected:
  boost::asio::io_service ios;
  boost::asio::ip::tcp::socket socket;
  boost::asio::ip::tcp::endpoint endpoint;
};

using namespace std::chrono_literals;

struct ServerFixture {

  ServerFixture() {
    server_thread = std::async(std::launch::async, [this]() {
      srv = new server(ios, 14488);
      ios.run();
    });
    std::this_thread::sleep_for(0.1s);
  }

  ~ServerFixture() {
    ios.stop();
    BOOST_REQUIRE_NO_THROW(server_thread.get());
    std::this_thread::sleep_for(0.1s);
  }
  server *srv;
  std::future<void> server_thread;
  boost::asio::io_service ios;
};

auto create_session(auto &client) {
  auto comm = std::make_unique<srp::netcomm>(std::move(client.Socket()));
  return srp::base_session::create_session(std::move(comm));
}

BOOST_AUTO_TEST_SUITE(Client_Create);

BOOST_FIXTURE_TEST_CASE(CreateClient, ServerFixture) {
  Client client(14488);
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(Client_Communication);

BOOST_FIXTURE_TEST_CASE(ClientInitMessage, ServerFixture) {
  Client client(14488);
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    rclient->init(48);
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
  delay(0.5s);

  BOOST_REQUIRE(srv->session_ptr()->messages().size() == 2);
  auto &msg = srv->session_ptr()->messages()[0].value();
  BOOST_REQUIRE(msg.action() == srp::ActionType::registration);

  auto repl = ProtoUtils::message_from_bytes<ClientRegistrationMessage>(msg.meta());
  BOOST_REQUIRE(repl.uid() == 48);
}

BOOST_FIXTURE_TEST_CASE(ClientCheckNoResp, ServerFixture) {
  Client client(14488);
  reply_on_message = false;
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    BOOST_REQUIRE_THROW(rclient->check().value(), std::bad_optional_access);
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_FIXTURE_TEST_CASE(ClientCheckResp, ServerFixture) {
  Client client(14488);
  reply_on_message = true;
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    auto check_result = rclient->check();
    auto check = ProtoUtils::message_from_bytes<ClientCheckResponse>(check_result.value().data());
    BOOST_REQUIRE(check.check_ok());
    BOOST_REQUIRE(check.info() == "Check all device");
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_FIXTURE_TEST_CASE(ClientStartResp, ServerFixture) {
  Client client(14488);
  reply_on_message = true;
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    auto check_result = rclient->start_recording("phub");

    auto check = ProtoUtils::message_from_bytes<ClientStartRecordResponse>(check_result.value().data());

    BOOST_REQUIRE(check.data_path().size() == 1);
    BOOST_REQUIRE(check.data_path()[0] == "phub.dat");
    BOOST_REQUIRE(check.sync_point_path().size() == 1);
    BOOST_REQUIRE(check.sync_point_path()[0] == "phub.sync");
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_FIXTURE_TEST_CASE(ClientStopResp, ServerFixture) {
  Client client(14488);
  reply_on_message = true;
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    auto check_result = rclient->stop_recording();

    auto check = ProtoUtils::message_from_bytes<ClientStopRecordResponse>(check_result.value().data());

    BOOST_REQUIRE(check.duration_sec() == 128.0);
    BOOST_REQUIRE(check.frames() == 800);

    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_FIXTURE_TEST_CASE(ClientFullLifeTime, ServerFixture) {
  Client client(14488);
  reply_on_message = true;
  auto base_session = create_session(client);
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));

    rclient->init(42);
    rclient->check();
    rclient->start_recording("demo_dat.txt");
    for (auto i = 0; i < 100; ++i) rclient->sync_time(i);
    rclient->stop_recording();

    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }
}

BOOST_AUTO_TEST_SUITE_END();