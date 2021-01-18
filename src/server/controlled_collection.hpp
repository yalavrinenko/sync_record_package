//
// Created by yalavrinenko on 19.01.2021.
//

#ifndef SRP_CONTROLLED_COLLECTION_HPP
#define SRP_CONTROLLED_COLLECTION_HPP

#include <net/sessions.hpp>
#include <server/device_collection.hpp>

namespace srp{
  class controlled_device_collection : public device_collection{
  public:
    static std::shared_ptr<controlled_device_collection> create_collection();

    static session_builder create_client_builder(auto collection);

    static session_builder create_monitor_builder(auto collection);

  protected:
    controlled_device_collection() = default;

  private:
    static size_t client_id_counter_;
  };
}

#endif//SRP_CONTROLLED_COLLECTION_HPP
