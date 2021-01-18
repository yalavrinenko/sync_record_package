//
// Created by yalavrinenko on 17.12.2020.
//

#ifndef SRP_REC_CLIENT_HPP
#define SRP_REC_CLIENT_HPP

#include <memory>
#include "../capture_interface/capture.hpp"
#include <session.pb.h>

namespace srp{

  class base_session;

  class recording_client: public capture_i {
  public:
    recording_client() = delete;
    void init(size_t uid) override;

    std::optional<ClientStartRecordResponse> start_recording(std::string const &path_template) override;

    std::optional<ClientStopRecordResponse> stop_recording() override;

    std::optional<ClientSyncResponse> sync_time(size_t sync_point) override;

    std::optional<ClientCheckResponse> check() override;

    std::optional<ClientTimeResponse> state() override;

    virtual bool is_connected();

    static std::unique_ptr<recording_client> from_base_session(std::shared_ptr<base_session> session);

    ~recording_client() override;
  protected:
    explicit recording_client(std::shared_ptr<base_session> session);

  private:
    struct recording_client_impl;

    std::unique_ptr<recording_client_impl> pimpl_;
  };

}

#endif//SRP_REC_CLIENT_HPP
