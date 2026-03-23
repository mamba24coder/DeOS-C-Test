
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

#include "include/string_uaf.h"
#include "include/sink_callback.h"
#include "include/shutdown.h"
#include "include/timebase.h"
#include "include/level.h"

struct Args {
  int seconds = 5;
  bool stress = false;
};

static Args parse_args(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
      a.seconds = std::atoi(argv[++i]);
    } else if (std::strcmp(argv[i], "--stress") == 0) {
      a.stress = true;
    }
  }
  return a;
}

int main(int argc, char** argv) {
  auto args = parse_args(argc, argv);
  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("output.log", true);
  auto tp = std::make_shared<spdlog::details::thread_pool>(8192, std::thread::hardware_concurrency());
  auto logger = std::make_shared<spdlog::async_logger>(
      "lab",
      spdlog::sinks_init_list{file_sink},
      tp,
      spdlog::async_overflow_policy::overrun_oldest);

  
  spdlog::set_level(spdlog::level::info);

  std::atomic<bool> stop{false};
  level::Level5 level5("level 5 logs");

  std::thread t1([&stop, stress = args.stress] { string_uaf::run(stop, stress); });
  std::thread t2([&stop, stress = args.stress] { timebase::run(stop, stress); });
  std::thread t3([&stop, stress = args.stress] { level::run(stop, stress); });
  std::thread t4([&stop, stress = args.stress] { shutdown::run(stop, stress); });
  std::thread t5([&stop, stress = args.stress] { sink_callback::run(stop, stress); });

  auto end = std::chrono::steady_clock::now() + std::chrono::seconds(args.seconds);
  while (std::chrono::steady_clock::now() < end) {
    spdlog::info("heartbeat");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    level5.print();
  }

  stop.store(true);
  t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

  spdlog::set_default_logger(nullptr);// release the shared ptr reference
  spdlog::shutdown();
  return 0;
}
