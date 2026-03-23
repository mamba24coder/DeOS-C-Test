#pragma once
#include <spdlog/spdlog.h>
#include <atomic>
#include <thread>

namespace shutdown {
inline void run(std::atomic<bool>& stop, bool stress) {
  auto logger = spdlog::default_logger_raw(); // shared_ptr
  std::thread t([&stop, logger] {
    while (!stop.load()) raw->info("logging...");
  }); // capture by value to keep ref count positive


  stop.store(true);//make sure thread stops before shutdown
  t.join();

  
  if (stress) {
    spdlog::shutdown();
  }
}
}
