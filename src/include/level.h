
#pragma once
#include <spdlog/spdlog.h>
#include <atomic>
#include <thread>
#include <string>

namespace level {
inline std::atomic<int> g_level = 0; //prevent race

class Level5
{
public:
  Level5(std::string ss): s(std::move(ss)) {};
  ~Level5() {}; //Removed delete &s

  void print() {
    spdlog::debug(s);
  }

private:
  std::string s;
};

inline void run(std::atomic<bool>& stop, bool) {
  std::thread w([&] {
    while (!stop.load()) g_level++;
  });
  std::thread r([&] {
    while (!stop.load()) {
      if (g_level & 1) spdlog::info("odd");
      else spdlog::debug("even");
    }
  });
  w.join();
  r.join();
}
}
