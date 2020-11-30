//
// Created by yalavrinenko on 30.11.2020.
//

#ifndef SRP_LOGGER_HPP
#define SRP_LOGGER_HPP

#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#define LOGI BOOST_LOG_TRIVIAL(info)
#define LOGE BOOST_LOG_TRIVIAL(error)
#define LOGW BOOST_LOG_TRIVIAL(warning)
#define LOGD BOOST_LOG_TRIVIAL(debug)

namespace srp{
  struct logger{
    static void init() {}
  };
}

#endif //SRP_LOGGER_HPP
