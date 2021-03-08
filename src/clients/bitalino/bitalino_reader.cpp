//
// Created by yalavrinenko on 28.01.2021.
//

#include "bitalino_reader.hpp"
#include <thread>
#include <sstream>
#include <iostream>
#include <utils/logger.hpp>

srp::bitalino_reader::bitalino_reader(std::string addr, size_t freq, size_t block_size, std::vector<int> channels)
    : data_block_(block_size), addr_{std::move(addr)}, sampling_rate_(freq), channels_{std::move(channels)} {
  if (addr_.empty()){
    LOGW << "No device address specified. Try to search bitalino devices...";

    auto devs = device_->find();

    LOGW << "Find " << devs.size() << " Bitalino devices:";
    for (auto &dev : devs){
      LOGW << "\t" << dev.name << "\t" << dev.macAddr;
    }

    throw std::logic_error("No device address.");
  }
  device_ = std::make_unique<BITalino>(addr_.c_str());
}
std::pair<std::chrono::high_resolution_clock::time_point, BITalino::VFrame const &> srp::bitalino_reader::acquire() {
  if (device_) {
    auto readed = device_->read(data_block_);

    if (static_cast<int>(data_block_.size()) == readed)
      return {std::chrono::high_resolution_clock::now(), data_block_};
  }
  throw BITalino::Exception(BITalino::Exception::Code::CONTACTING_DEVICE);
}
void srp::bitalino_reader::start(bool debug_mode) {
  if (device_) device_->start(sampling_rate_, channels_, debug_mode);
}

void srp::bitalino_reader::stop() {
  if (device_) device_->stop();
}
std::string srp::bitalino_reader::device_info() const {
  if (!device_)
    return {};

  std::ostringstream oss;
  oss << "Bitalino device with address " << addr_ << ". Device version " << device_->version() <<".";
  return oss.str();
}
srp::bitalino_reader::~bitalino_reader() {
  device_.reset(nullptr);
}
