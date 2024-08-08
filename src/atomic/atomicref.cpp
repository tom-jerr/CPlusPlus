#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <random>
#include <stop_token>
#include <syncstream>
#include <thread>
#include <vector>
using namespace std::literals;
inline auto syncOut(std::ostream &strm = std::cout) {
  return std::osyncstream{strm};
}

int main() {
  std::array<int, 100> values;
  std::fill_n(values.begin(), values.size(), 100);

  std::stop_source stop_source;
  std::stop_token token{stop_source.get_token()};

  std::vector<std::jthread> threads;
  for (int i = 0; i < 9; ++i) {
    threads.push_back(std::jthread{
        [&values](std::stop_token st) {
          std::mt19937 eng{std::random_device{}()};
          std::uniform_int_distribution<int> dist{0, values.size() - 1};
          while (!st.stop_requested()) {
            int idx = dist(eng);

            std::atomic_ref val{values[idx]};
            --val;
            if (val <= 0) {
              std::cout << "Value at index " << idx << " is now 0\n";
            }
          }
        },
        token});
  }

  std::this_thread::sleep_for(1s);
  syncOut() << "Requesting stop...\n";
  stop_source.request_stop();
}