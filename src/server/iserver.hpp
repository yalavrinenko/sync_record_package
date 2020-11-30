//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_ISERVER_HPP
#define SRP_ISERVER_HPP
namespace srp{
  class iserver{
  public:
    virtual void start() = 0;

    virtual void stop() = 0;
  };
}
#endif //SRP_ISERVER_HPP
