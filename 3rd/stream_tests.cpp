//
// Created by yalavrinenko on 29.12.2020.
//
#include "session.pb.h"
#include <boost/asio.hpp>
#include <future>
#include <google/protobuf/util/json_util.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <iostream>

using namespace boost::asio::ip;
using namespace std::chrono_literals;

struct iostream_handler{
  tcp::iostream iss;
  //google::protobuf::io::OstreamOutputStream proto_os;
  google::protobuf::io::IstreamInputStream proto_is;
  bool proto_eof = false;
  explicit iostream_handler(tcp::socket socket): iss{std::move(socket)}, proto_is(&iss){
  }

  bool alive() { return iss || !proto_eof; }
};

auto to_json(auto const &message) {
  std::string str;
  auto status = google::protobuf::util::MessageToJsonString(message, &str);
  return str;
};

void send_message(auto const &message, iostream_handler &handler){
  google::protobuf::util::SerializeDelimitedToOstream(message, &(handler.iss));
  handler.iss.flush();
  std::cout << "Send " << to_json(message) << std::endl;
}

template<typename T>
T recv_message(iostream_handler &handler){
  T message;
  google::protobuf::util::ParseDelimitedFromZeroCopyStream(&message, &(handler.proto_is), &(handler.proto_eof));
  return message;
}

class Server {
public:
  static void run() {
    boost::asio::io_service ios;
    auto io = std::async(std::launch::async, [&ios]() { ios.run(); });

    tcp::acceptor acp(ios, tcp::endpoint{tcp::v4(), 1428});

    tcp::socket sock{ios};

    acp.accept(sock);

    iostream_handler ss{std::move(sock)};

    while (ss.alive()) {
      auto act = recv_message<srp::ClientActionMessage>(ss);
      std::cout << "Recv: " << to_json(act) << std::endl;
//      act.set_action(srp::ActionType(act.action() + 1));
//      send_message(act, ss);
    }

    io.get();
  }
};

class Client {
public:
  static void run() {
    boost::asio::io_service ios;
    auto io = std::async(std::launch::async, [&ios]() { ios.run(); });

    tcp::socket sock{ios};
    sock.connect(tcp::endpoint{boost::asio::ip::address::from_string("127.0.0.1"), 1428});

    iostream_handler ss(std::move(sock));
    auto i = 1;
    while (ss.iss && i < 10) {
      srp::ClientActionMessage act;
      act.set_action(srp::ActionType(i % 6));
      ++i;

      send_message(act, ss);

//      auto ract = recv_message<srp::ClientActionMessage>(ss);
//      std::cout << "CRecv: " << to_json(act) << std::endl;

      if (i == 6)
        std::this_thread::sleep_for(10s);
    }

    io.get();
  }
};

int main(int argc, char **argv) {
  auto server = std::async(std::launch::async, []() { Server::run(); });


  auto client = std::async(std::launch::async, []() { Client::run(); });
  server.get();
  client.get();
  return 0;
}