//
// Created by yalavrinenko on 30.11.2020.
//
#include "../utils/logger.hpp"
#include "control_server.hpp"
#include <server/controlled_collection.hpp>

int main(int argc, char** argv){
  srp::ControlServerOption sopt;

  srp::control_server server(srp::control_server::connection_point{
    .host = "",
    .port = 14488
  });

  auto collection = srp::controlled_device_collection::create_collection();

  server.register_session_acceptor(srp::SessionType::client, srp::controlled_device_collection::create_client_builder(collection));
  server.register_session_acceptor(srp::SessionType::ui, srp::controlled_device_collection::create_monitor_builder(collection));
  server.register_session_acceptor(srp::SessionType::master, srp::controlled_device_collection::create_master_builder(collection));

  server.start();
  return 0;
}