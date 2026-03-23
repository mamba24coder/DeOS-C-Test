
A tiny project that uses spdlog.
Build:
  cmake -S . -B build
  cmake --build build -j
  ./build/spdlogger --seconds 5 (optional --stress)


Your task 
Identify and fix as many correctness issues as you reasonably can. 
For each fix, briefly explain: 
what the problem was 
why it happens (especially why it’s intermittent) 
why your fix is correct 

Constraints 
Do not remove async logging. 

Upload solutions to github with Readme 




Solution:
The issues and fixes are identified per file, starting from CMakeLists.txt, main.cpp and then each of the header files in src/include

1- CMakeLists.txt:

Issue: CMakeLists.txt only knows about src/main.cpp. It does not know where to find the headers included in main.cpp (e.g., #include "include/string_uaf.h"). 
This will cause a compilation error.

Fix: Add target_include_directories
target_include_directories(spdlogger PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

Issue: We are dealing with a mutithreaded C++ application using std::thread. 
Fix: Add Threads support in CMakeLists
target_link_libraries(spdlogger PRIVATE spdlog::spdlog Threads::Threads)

2- main.cpp

Issue: We create a thread_pool (tp) and an async_logger(logger), then set it as the default logger (spdlog::set_default_logger(logger);)
However, since spdlog::shutdown() is called at the end of main(), there might be a case where the worker threads (t1-t5) or the level5 object attempt to log after
main begins its exit sequence but before the threads join. In that case the threads might reference a logger or a thread pool that is being destroyed. 

Fix: Drop the global logger reference before shutting down to ensure all pending messages are flushed and handles are released. 
spdlog::set_default_logger(nullptr);

Issue: The thread pool is initialized with 8192 slots and 1 worker thread. A single thread can quickly fall behind and because overrun_oldest is used, we can silently this
way lose logs unknowingly. 
Fix : Increase worker thread count or use block policy if data integrity if data integrity is a higher priority than low-latency.
auto tp = std::make_shared<spdlog::details::thread_pool>(8192, std::thread::hardware_concurrency());


Issue: We are capturing stop and args  by reference [&] in 5 different threads. 
While stop is an std::atomic, args is a local struct. If main were to exit prematurely, these threads would be left with dangling references to a stack
that no longer exists.

Fix: Pass args.stress  by value to the lambda to ensure the thread owns its data.
std::thread t2([&stop, stress = args.stress] { timebase::run(stop, stress); });



3- string_uaf.h

Issue:
d.set() creates a local std::string tmp, assigns its internal pointer to ptr, and then tmp is destroyed at the end of the function. 
d.emit() then tries to read that deleted memory. 

Fix:
Store a std::string inside the struct so the data remains valid for the life of the object

4- sink_callback.h

Issue:
Inside sink_it_, we call spdlog::info. This triggers the logger, which calls the sink, which call the logger creating an infinite recursion that crashes the stack
or deadlocks the mutex.
Fix: 
Sinks should never call the logger they are attached to. We can use a separate logger or raw print call for debugging sinks.
void sink_it_(const spdlog::details::log_msg&) override {
	fmt::print("Sink message received");
  }
  
5- shutdown.h

Issue: 
The thread t uses a raw pointer to the logger while the spdlog:shutdown() is called in the same function.
Once shutdown is called, the logger is destroyed and the thread crashes trying to call raw->info()

Fix:
Make the thread to stop before calling shutdown and use a shared_ptr to keep the logger alive while in use

auto logger = spdlog::default_logger_raw(); // shared_ptr
  std::thread t([&stop, logger] {
    while (!stop.load()) raw->info("logging...");
  }); // capture by value to keep ref count positive


  stop.store(true);//make sure thread stops before shutdown
  t.join();

  
  if (stress) {
    spdlog::shutdown();
  }

6- timebase.h

Issue: We are substracting a system_clock timestamp from a steady_clock timestamp. These clocks have different epoch starting points.
The result will be a massive, meaningless number.

Fix: Use the same clock for both points.
auto t1 = std::chrono::steady_clock::now();
std::this_thread::sleep_for(std::chrono::milliseconds(1));
auto t2 = std::chrono::steady_clock::now();



7- level.h

Issue: 
delete &s; tries to delete an object that was not allocated with new keyword. This is memory corruption.

Fix: Remove the invalid delete 

Issue: g_level is a raw int accessed by 2 threads without a mutex or std::atomic thus resulting in a data race.
Fix: Make the counter atomic 

