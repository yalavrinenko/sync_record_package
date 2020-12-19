//
// Created by yalavrinenko on 17.12.2020.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "../src/server/rec_client.hpp"
#include <boost/test/unit_test.hpp>
#include "../src/protocols/actions.hpp"
#include <google/protobuf/util/json_util.h>
#include "../src/server/sessions.hpp"

using boost::asio::ip::tcp;

class session {
public:
  session(boost::asio::io_service &io_service) : socket_(io_service) {}

  tcp::socket &socket() { return socket_; }

  void start() {
    sync_read_thread_ = std::async(std::launch::async, [this](){
      this->sync_read();
    });
  }

  auto const& messages() const { return messages_; }
private:

  void sync_read(){
    std::optional<srp::ClientActionMessage> message;
    do{
      message = srp::NetUtils::sync_read_proto<srp::ClientActionMessage>(socket_);
      if (message) {
        std::string str;
        auto status = google::protobuf::util::MessageToJsonString(message.value(), &str);
        std::cout << status.message() << ":\t" << str << std::endl;
        messages_.emplace_back(message);
      }
    }
    while (message);
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  std::future<void> sync_read_thread_;
  std::vector<std::optional<srp::ClientActionMessage>> messages_;
};

class server {
public:
  server(boost::asio::io_service &io_service, short port) : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
  }

  auto& session_ptr() { return session_; }

private:
  void start_accept() {
    session_ = std::make_unique<session>(io_service_);
    acceptor_.async_accept(session_->socket(), [this](auto v2){
      this->handle_accept(session_, v2);
    });
  }

  void handle_accept(std::unique_ptr<session> &new_session, const boost::system::error_code &error) {
    if (!error) {
      new_session->start();
    }
  }

  std::unique_ptr<session> session_;
  boost::asio::io_service &io_service_;
  tcp::acceptor acceptor_;
};

class Client {
public:
  Client(unsigned short port) : socket{ios}, endpoint{boost::asio::ip::address::from_string("127.0.0.1"), port} { socket.connect(endpoint); }

  auto &Socket() { return socket; }

  ~Client() { }

protected:
  boost::asio::io_service ios;
  boost::asio::ip::tcp::socket socket;
  boost::asio::ip::tcp::endpoint endpoint;
};

using namespace std::chrono_literals;

BOOST_AUTO_TEST_SUITE(Client_Create);

BOOST_AUTO_TEST_CASE(CreateClient){
  boost::asio::io_service ios;

  auto server_thread = std::async(std::launch::async, [&ios](){
    server s(ios, 14488);
    ios.run();
  });

  std::this_thread::sleep_for(1s);

  Client client(14488);
  auto base_session = srp::base_session::create_session(std::move(client.Socket()));
  {
    std::unique_ptr<srp::recording_client> rclient;
    BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
    BOOST_REQUIRE_NO_THROW(rclient.reset());
  }

  std::this_thread::sleep_for(1s);
  ios.stop();

  BOOST_REQUIRE_NO_THROW(server_thread.get());


}

  BOOST_AUTO_TEST_CASE(ClientInitMessage){
    boost::asio::io_service ios;

    auto server_thread = std::async(std::launch::async, [&ios](){
      server s(ios, 14488);
      ios.run();
    });

    std::this_thread::sleep_for(1s);

    Client client(14488);
    auto base_session = srp::base_session::create_session(std::move(client.Socket()));
    {
      std::unique_ptr<srp::recording_client> rclient;
      BOOST_REQUIRE_NO_THROW(rclient = srp::recording_client::from_base_session(std::move(base_session)));
      rclient->init(48);
      BOOST_REQUIRE_NO_THROW(rclient.reset());
    }

    std::this_thread::sleep_for(1s);
    ios.stop();

    BOOST_REQUIRE_NO_THROW(server_thread.get());
  }

BOOST_AUTO_TEST_SUITE_END();