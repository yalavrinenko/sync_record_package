//
// Created by yalavrinenko on 12.01.2021.
//

#ifndef SRP_EXCEPTIONS_HPP
#define SRP_EXCEPTIONS_HPP
#include <exception>

namespace srp{
class context_allocation_fail : public std::exception{
public:
  [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
    return "Fail to allocate av_context!";
  }
};

class context_open_fail : public std::exception{
public:
  explicit context_open_fail(int code) : code_{code}{}

  [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
    return "Fail to open av_context!";
  }

  auto code() const noexcept {
    return code_;
  }

private:
  int code_;
};

  class codec_open_fail : public std::exception{
  public:
    explicit codec_open_fail() {}

    [[nodiscard]] const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW override {
      return "Fail to open codec!";
    }
  };
}

#endif//SRP_EXCEPTIONS_HPP
