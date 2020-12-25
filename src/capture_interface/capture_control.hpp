//
// Created by yalavrinenko on 25.12.2020.
//

#ifndef SRP_CAPTURE_CONTROL_HPP
#define SRP_CAPTURE_CONTROL_HPP
#include <memory>
namespace srp{
  class controller_i{
  public:

  };

  using capture_controller = std::unique_ptr<controller_i>;
}
#endif//SRP_CAPTURE_CONTROL_HPP
