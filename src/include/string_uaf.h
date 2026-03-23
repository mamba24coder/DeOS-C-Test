
#pragma once
#include <spdlog/spdlog.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

namespace string_uaf {

struct DeferredLog {
  str::string data;
  size_t len;

  void set() {
    data = "uaf:" + std::to_string(std::rand());
  }

  void emit() const {
    spdlog::info("deferred={}", data);
  }
};

inline void run(std::atomic<bool>& stop, bool stress) {
  while (!stop.load()) {
    DeferredLog d;
    d.set();
    std::this_thread::sleep_for(std::chrono::microseconds(stress ? 50 : 500));
    d.emit();
    std::this_thread::sleep_for(std::chrono::milliseconds(stress ? 1 : 10));
  }
}
}
