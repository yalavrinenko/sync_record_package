//
// Created by yalavrinenko on 30.11.2020.
//
#include <utils/logger.hpp>
#include <utils/io.hpp>
#include "control_server.hpp"
#include "options.pb.h"
#include <server/controlled_collection.hpp>

int main(int argc, char** argv){
#ifdef WIN32
  //std::setlocale(LC_ALL, ".1251");
  //std::locale::global(std::locale(""));
  std::cout << "KOI test: " << " Тест поддержки кирилицы" << std::endl;
#endif

  if (argc < 2){
    LOGE << "No config file. Use ./sync_capture [option].json";
    return 1;
  }



  auto sopt = srp::ProtoUtils::message_from_json<srp::ControlServerOption>(srp::IoUtils::read_json(argv[1]));

  if (!sopt){
    LOGE << "Fail to load server configuratin. Terminate.";
    return 1;
  }

  srp::control_server server(srp::control_server::connection_point{
    .host = sopt->host(),
    .port = static_cast<unsigned int>(sopt->port())
  });

  auto collection = srp::controlled_device_collection::create_collection();

  server.register_session_acceptor(srp::SessionType::client, srp::controlled_device_collection::create_client_builder(collection));
  server.register_session_acceptor(srp::SessionType::ui, srp::controlled_device_collection::create_monitor_builder(collection));
  server.register_session_acceptor(srp::SessionType::master, srp::controlled_device_collection::create_master_builder(collection));

  server.start();
  return 0;
}