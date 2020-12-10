//
// Created by yalavrinenko on 10.12.2020.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <future>
#include <chrono>

#include "../src/utils/io.hpp"
#include "../src/server/control_server.hpp"

using namespace std::chrono_literals;


BOOST_AUTO_TEST_SUITE(Server_Launch);

BOOST_AUTO_TEST_CASE(Normal_Start_Stop){
    srp::control_server server(srp::control_server::connection_point{
        .host = "",
        .port = 14488
    });

    auto server_thread = std::async(std::launch::async, [&server]() { server.start(); });
    std::this_thread::sleep_for(2s);
    BOOST_CHECK_NO_THROW(server.stop());
    BOOST_CHECK_NO_THROW(server_thread.get());
}

  BOOST_AUTO_TEST_CASE(Start_stop_at_port_22){
    srp::control_server server(srp::control_server::connection_point{
        .host = "",
        .port = 22
    });

    auto server_thread = std::async(std::launch::async, [&server]() { server.start(); });
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_NO_THROW(server.stop());
    BOOST_CHECK_THROW(server_thread.get(), srp::acceptor_create_error);
  }

BOOST_AUTO_TEST_SUITE_END();