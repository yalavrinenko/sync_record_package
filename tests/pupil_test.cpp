//
// Created by yalavrinenko on 07.03.2021.
//
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
#include <../src/utils/logger.hpp>
#include <../src/clients/eye_tracker/pupil_eye_io.hpp>
#include <thread>
#include <../src/utils/io.hpp>

BOOST_AUTO_TEST_SUITE(PupilMain);

BOOST_AUTO_TEST_CASE(ConnectStartStop){
  srp::pupil_eye_io pupil("127.0.0.1", 50020);

  LOGD << pupil.name() << std::endl;
  LOGD << pupil.start_recording("/home/yalavrinenko/Files/git/sync_record_package/tests/output/eye_01");
  using namespace std::chrono_literals;
  for (auto i = 0; i < 10; ++i) {
    std::this_thread::sleep_for(1s);
    std::cout << pupil.timestamp().count() << std::endl;
  }
  LOGD << pupil.stop_recording().count();

}

BOOST_AUTO_TEST_CASE(PathUtilsTest){
  auto [out, stp] = srp::PathUtils::create_file_path("/", "pathtmp", "0", "wav");
  LOGD << out.string();
  LOGD << stp.string();
}

BOOST_AUTO_TEST_SUITE_END();