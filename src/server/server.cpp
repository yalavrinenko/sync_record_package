//
// Created by yalavrinenko on 30.11.2020.
//
#include "../utils/logger.hpp"
#include "control_server.hpp"

int main(int argc, char** argv){
  srp::control_server server(srp::control_server::connection_point{
    .host = "",
    .port = 14488
  });

  server.start();
  return 0;
}