//
// Created by yalavrinenko on 10.12.2020.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <future>

#include "../src/server/control_server.hpp"
#include "../src/server/server_acceptor.hpp"
#include "../src/utils/io.hpp"

using namespace std::chrono_literals;


BOOST_AUTO_TEST_SUITE(Server_Launch);

BOOST_AUTO_TEST_CASE(Normal_Start_Stop) {
  srp::control_server server(srp::control_server::connection_point{.host = "", .port = 14488});

  auto server_thread = std::async(std::launch::async, [&server]() { server.start(); });
  std::this_thread::sleep_for(2s);
  BOOST_CHECK_NO_THROW(server.stop());
  BOOST_CHECK_NO_THROW(server_thread.get());
}

BOOST_AUTO_TEST_CASE(Start_stop_at_port_22) {
  srp::control_server server(srp::control_server::connection_point{.host = "", .port = 22});

  auto server_thread = std::async(std::launch::async, [&server]() { server.start(); });
  std::this_thread::sleep_for(1s);
  BOOST_CHECK_NO_THROW(server.stop());
  BOOST_CHECK_THROW(server_thread.get(), srp::acceptor_create_error);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(Connection);

class TestServer : public srp::control_server {
public:
  TestServer(unsigned int port) : srp::control_server(srp::control_server::connection_point{.host = "", .port = port}) {
    std::cout << "Test server" << std::endl;
  }
};

class Client {
public:
  Client(unsigned short port) : socket{ios}, endpoint{boost::asio::ip::address::from_string("127.0.0.1"), port} { socket.connect(endpoint); }

  auto &Socket() { return socket; }

  ~Client() { socket.close(); }

protected:
  boost::asio::io_service ios;
  boost::asio::ip::tcp::socket socket;
  boost::asio::ip::tcp::endpoint endpoint;
};

BOOST_AUTO_TEST_CASE(Single_connection) {
  TestServer srv(14488);
  auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
  std::this_thread::sleep_for(1s);

  {
    std::unique_ptr<Client> c;
    BOOST_CHECK_NO_THROW(c = std::make_unique<Client>(14488));
    std::this_thread::sleep_for(1s);
  }

  BOOST_CHECK_NO_THROW(srv.stop());
  BOOST_CHECK_NO_THROW(wthread.get());
}

BOOST_AUTO_TEST_CASE(Single_connection_timeout) {
  TestServer srv(14489);
  auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
  std::this_thread::sleep_for(1s);

  std::unique_ptr<Client> c;
  BOOST_CHECK_NO_THROW(c = std::make_unique<Client>(14489));

  std::this_thread::sleep_for(10s);

  BOOST_CHECK_NO_THROW(srv.stop());
  BOOST_CHECK_NO_THROW(wthread.get());
}

BOOST_AUTO_TEST_CASE(Send_valid_message) {

  TestServer srv(14490);
  auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
  std::this_thread::sleep_for(1s);

  for (auto ctype : {srp::SessionType::client, srp::SessionType::ui, srp::SessionType::undefined}) {
    std::unique_ptr<Client> c;
    BOOST_CHECK_NO_THROW(c = std::make_unique<Client>(14490));

    srp::ClientWelcomeMessage mess;
    mess.set_type(ctype);

    srp::NetUtils::sync_send_proto(c->Socket(), mess);
  }


  std::this_thread::sleep_for(10s);
  BOOST_CHECK_NO_THROW(srv.stop());
  BOOST_CHECK_NO_THROW(wthread.get());
}

BOOST_AUTO_TEST_CASE(Send_stress_sync) {

  TestServer srv(14490);
  auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
  std::this_thread::sleep_for(1s);

  for (auto i = 0; i < 1000; ++i) {
    for (auto ctype : {srp::SessionType::client, srp::SessionType::ui, srp::SessionType::undefined}) {
      std::unique_ptr<Client> c;
      c = std::make_unique<Client>(14490);

      srp::ClientWelcomeMessage mess;
      mess.set_type(ctype);

      BOOST_REQUIRE(srp::NetUtils::sync_send_proto(c->Socket(), mess));
    }
  }

  std::this_thread::sleep_for(1s);
  BOOST_CHECK_NO_THROW(srv.stop());
  BOOST_CHECK_NO_THROW(wthread.get());
}

BOOST_AUTO_TEST_CASE(Send_invalid_message) {

    TestServer srv(14490);
    auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
    std::this_thread::sleep_for(1s);

    {
      std::unique_ptr<Client> c;
      BOOST_CHECK_NO_THROW(c = std::make_unique<Client>(14490));

      std::string message = "xyi";
      size_t size = message.size();
      BOOST_REQUIRE(srp::NetUtils::sync_send_message(c->Socket(), boost::asio::buffer(&size, sizeof(size))));
      BOOST_REQUIRE(srp::NetUtils::sync_send_message(c->Socket(), boost::asio::buffer(message)));
    }

    {
      std::unique_ptr<Client> c;
      BOOST_CHECK_NO_THROW(c = std::make_unique<Client>(14490));

      std::string message = "massive xyi";
      BOOST_REQUIRE(srp::NetUtils::sync_send_message(c->Socket(), boost::asio::buffer(message)));
    }

    std::this_thread::sleep_for(1s);
    BOOST_CHECK_NO_THROW(srv.stop());
    BOOST_CHECK_NO_THROW(wthread.get());
  }

BOOST_AUTO_TEST_CASE(Send_stress_assync) {

    TestServer srv(14490);
    auto wthread = std::async(std::launch::async, [&srv]() { srv.start(); });
    std::this_thread::sleep_for(1s);

    auto stress_function = []() {
          for (auto i = 0; i < 50; ++i) {
            for (auto ctype : {srp::SessionType::client, srp::SessionType::ui, srp::SessionType::undefined}) {
              std::unique_ptr<Client> c;
              c = std::make_unique<Client>(14490);

              srp::ClientWelcomeMessage mess;
              mess.set_type(ctype);

              BOOST_REQUIRE(srp::NetUtils::sync_send_proto(c->Socket(), mess));
            }
          }
        };

    std::vector<std::future<void>> threads; threads.reserve(10);
    for (auto i = 0; i < 10; ++i)
      threads.emplace_back(std::async(std::launch::deferred, stress_function));

    for (auto &future: threads)
      BOOST_CHECK_NO_THROW(future.get());

    std::this_thread::sleep_for(1s);
    BOOST_CHECK_NO_THROW(srv.stop());
    BOOST_CHECK_NO_THROW(wthread.get());
  }


BOOST_AUTO_TEST_SUITE_END();