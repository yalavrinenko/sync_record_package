//
// Created by yalavrinenko on 28.01.2021.
//

#ifndef SRP_BITALINO_READER_HPP
#define SRP_BITALINO_READER_HPP

#include <bitalino.h>
#include <memory>
#include <chrono>

namespace srp{
  class bitalino_reader {
  public:
    bitalino_reader(std::string addr, size_t freq, size_t block_size, std::vector<int> channels);

    void start(bool debug_mode = false);
    std::pair<std::chrono::high_resolution_clock::time_point, BITalino::VFrame const&> acquire();
    void stop();

    [[nodiscard]] std::string device_info() const;

    [[nodiscard]] auto const& address() const { return addr_; }
    [[nodiscard]] auto block_size() const { return data_block_.size(); }
  private:
    std::unique_ptr<BITalino> device_;
    BITalino::VFrame data_block_;

    std::string addr_;
    size_t sampling_rate_;
    BITalino::Vint channels_;
  };
}

#endif//SRP_BITALINO_READER_HPP
