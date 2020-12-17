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

  socket_.async_read_some(boost::asio::buffer(data_, max_length),
                          [this](auto v1, auto v2) {
                            this->handle_read(v1, v2);
                          });
  }

private:
  void handle_read(const boost::system::error_code &error, size_t bytes_transferred) {
    if (!error) {
      srp::ClientActionMessage message;
      message.ParseFromArray(data_, bytes_transferred);
      std::string str;
      auto status = google::protobuf::util::MessageToJsonString(message, &str);
      std::cout << "[" << error.message() << "]\t\t" << status.message() << ":\t" << str << std::endl;

      socket_.async_read_some(boost::asio::buffer(data_, max_length),
                              [this](auto v1, auto v2) {
                                this->handle_read(v1, v2);
                              });
    } else {
      delete this;
    }
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server {
public:
  server(boost::asio::io_service &io_service, short port) : io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port)) {
    start_accept();
  }

private:
  void start_accept() {
    auto new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(), [this, new_session](auto v2){
      this->handle_accept(new_session, v2);
    });
  }

  void handle_accept(session *new_session, const boost::system::error_code &error) {
    if (!error) {
      new_session->start();
    } else {
      delete new_session;
    }

    start_accept();
  }

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

BOOST_AUTO_TEST_SUITE_END();