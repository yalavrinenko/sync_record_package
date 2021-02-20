//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_NETCLIENT_HPP
#define SRP_NETCLIENT_HPP

#include <memory>
#include <options.pb.h>
#include <vector>
#include <list>
#include <future>

namespace srp {

  class capture_i;

  class netclient {
  public:
    netclient();

    void run();
    void stop();

    [[maybe_unused]] bool add_and_run_client_instance(std::unique_ptr<srp::capture_i> capture, ControlServerOption const &target_opt);

    [[nodiscard]] size_t instances_count() const;

    ~netclient();

  private:
    class instance_handler;
    std::list<std::unique_ptr<instance_handler>> instances_;
    std::vector<std::future<void>> wthreads_;
  };
};


#endif//SRP_NETCLIENT_HPP
