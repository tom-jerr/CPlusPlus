#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>

using namespace std::literals;

int main() {
  std::queue<std::string> messages;
  std::mutex messagesMutex;
  std::condition_variable_any messagesCV;

  std::jthread t1{[&](std::stop_token st) {
    while (!st.stop_requested()) {
      std::string msg;
      {
        std::unique_lock lock{messagesMutex};
        if (!messagesCV.wait(lock, st, [&] { return !messages.empty(); })) {
          return;
        }
        msg = messages.front();
        messages.pop();
      }
      std::cout << "msg: " << msg << std::endl;
    }
  }}; // t1
  for (std::string s : {"msg1", "msg2", "msg3"}) {

    std::scoped_lock lock{messagesMutex};
    messages.push(s);

    messagesCV.notify_one();
  }

  std::this_thread::sleep_for(1s);
  t1.request_stop();
  {
    std::scoped_lock lock{messagesMutex};
    messages.push("msg4");
    messagesCV.notify_all();
  }

  std::this_thread::sleep_for(1s);
}