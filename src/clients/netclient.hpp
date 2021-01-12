//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_NETCLIENT_HPP
#define SRP_NETCLIENT_HPP

#include <memory>
#include <options.pb.h>
#include <vector>
namespace srp {

  class capture_i;

  class netclient {
  public:
    netclient();

    void run();
    void stop();

    [[maybe_unused]] bool add_client_instance(std::unique_ptr<srp::capture_i> capture, ControlServerOption const &target_opt);

    ~netclient();

  private:
    class instance_handler;
    std::vector<std::unique_ptr<instance_handler>> instances_;
  };
};


#endif//SRP_NETCLIENT_HPP
