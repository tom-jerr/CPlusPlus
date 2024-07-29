#include <chrono>
#include <future>
#include <iostream>
#include <stop_token>
#include <syncstream>
#include <thread>

using namespace std::literals;

auto syncOut(std::ostream &strm = std::cout) { return std::osyncstream{strm}; }

void task(std::stop_token st, int num) {
  auto id = std::this_thread::get_id();
  syncOut() << "call task (" << num << ") on thread: " << id << std::endl;
  // register a callback to stop the task
  std::stop_callback cb1{st, [num, id] {
                           syncOut() << "- STOP1 required in task (" << num
                                     << ") on thread: " << id << std::endl;
                         }};
  std::this_thread::sleep_for(9ms);

  // register another callback to stop the task
  std::stop_callback cb2{st, [num, id] {
                           syncOut() << "- STOP2 required in task (" << num
                                     << ") on thread: " << id << std::endl;
                         }};
  std::this_thread::sleep_for(2ms);
}

int main() {
  // create a stop_source
  std::stop_source stSrc;
  std::stop_token stok{stSrc.get_token()};

  // register a callback to stop the main thread
  std::stop_callback cb1{
      stok, [] { syncOut() << "- STOP required in main" << std::endl; }};
  auto future = std::async([stok] {
    for (int num = 1; num < 10; ++num) {
      task(stok, num);
    }
  });

  std::this_thread::sleep_for(10ms);
  stSrc.request_stop();
}