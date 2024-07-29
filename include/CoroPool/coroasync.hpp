#include "coropool.hpp"
#include <iostream>

#include <syncstream>

inline auto syncOut(std::ostream &strm = std::cout) {
  return std::osyncstream{strm};
}

CoroTask print(std::string id, std::string msg) {
  syncOut() << " > " << id << " print: " << msg
            << " on thread: " << std::this_thread::get_id() << std::endl;
  co_return;
}

CoroTask runAsync(std::string id) {
  syncOut() << " ==== " << id
            << " start on thread: " << std::this_thread::get_id() << std::endl;
  co_await print(id, "task 1");
  syncOut() << " ==== " << id << " resume: " << std::this_thread::get_id()
            << std::endl;
  co_await print(id, "task 2");
  syncOut() << " ==== " << id << " resume: " << std::this_thread::get_id()
            << std::endl;
  syncOut() << " ==== " << id << " done" << std::endl;
}