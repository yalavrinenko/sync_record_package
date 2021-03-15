//
// Created by yalavrinenko on 07.03.2021.
//

#ifndef SRP_PUPIL_EYE_IO_HPP
#define SRP_PUPIL_EYE_IO_HPP

#include <chrono>
#include <memory>
#include <filesystem>

namespace srp {
  class pupil_eye_io {
  public:
    pupil_eye_io(std::string host, long port);

    [[nodiscard]] std::string name() const;
    [[nodiscard]] std::chrono::milliseconds timestamp() const;

    std::string start_recording(std::filesystem::path const& path);

    std::chrono::milliseconds stop_recording();

    ~pupil_eye_io();
  protected:
    class pupil_core_impl;
    std::unique_ptr<pupil_core_impl> core_;
  };
}


#endif//SRP_PUPIL_EYE_IO_HPP
