#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace std::literals;

template <typename T> class AtomicList {
private:
  struct Node {
    T val;
    std::shared_ptr<Node> next;
  };
  std::atomic<std::shared_ptr<Node>> head;

public:
  AtomicList() = default;

  void insert(T v) {
    auto p = std::make_shared<Node>();
    p->val = v;
    p->next = head;
    while (!head.compare_exchange_weak(p->next, p)) { // atomic update}
    }
  }

  void print() const {
    std::cout << "HEAD";
    for (auto p = head.load(); p; p = p->next) {
      std::cout << " -> " << p->val;
    }
    std::cout << " -> TAIL\n";
  }
};

int main() {
  AtomicList<std::string> alist;

  {
    std::vector<std::jthread> threads;
    for (int i = 0; i < 10; ++i) {
      threads.push_back(std::jthread{[&, i] {
        for (auto s : {"A"s, "B"s, "C"s, "D"s, "E"s}) {
          alist.insert(s + std::to_string(i));
          std::this_thread::sleep_for(100ms);
        }
      }});
    }
  } // wait for threads to join
  alist.print();
}
