//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_REC_CLIENT_HPP
#define SRP_REC_CLIENT_HPP

#include <memory>
#include "../capture_interface/capture.hpp"
namespace srp{

  class base_session;

  class recording_client: public capture_i {
  public:
    recording_client() = delete;
    void init(size_t uid) override;
    void start_recording() override;
    void stop_recording() override;
    void sync_time() override;
    void check() override;

    virtual bool is_connected();

    static std::unique_ptr<recording_client> from_base_session(std::shared_ptr<base_session> session);

    ~recording_client();
  protected:
    explicit recording_client(std::shared_ptr<base_session> session);

  private:
    struct recording_client_impl;

    std::unique_ptr<recording_client_impl> pimpl_;
  };

}

#endif//SRP_REC_CLIENT_HPP
