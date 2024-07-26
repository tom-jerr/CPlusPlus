#include <iostream>
#include <memory>
#include <vector>

struct MoveMsg {
  int x;
  int y;

  void speak() { std::cout << "Move " << x << ", " << y << '\n'; }

  void happy() { std::cout << "Happy " << x << ", " << y << '\n'; }
};
struct JumpMsg {
  int height;

  void speak() { std::cout << "Jump " << height << '\n'; }
};
struct SleepMsg {
  int time;
  void speak() { std::cout << "Sleep " << time << '\n'; }
};

struct MsgBase {
  virtual void speak() = 0;
  virtual ~MsgBase() = default;
  virtual std::shared_ptr<MsgBase> clone() const = 0;
  virtual void happy() = 0;
  using Ptr = std::shared_ptr<MsgBase>;
};

template <class Msg> struct MsgImpl : MsgBase {
  Msg msg;

  template <class... Ts> MsgImpl(Ts &&...ts) : msg{std::forward<Ts>(ts)...} {}

  void speak() override { msg.speak(); }

  void happy() override {
    if constexpr (requires { msg.happy(); }) {
      msg.happy();
    } else {
      std::cout << "Not happy\n";
    }
  }

  std::shared_ptr<MsgBase> clone() const override {
    return std::make_shared<MsgImpl<Msg>>(msg);
  }
};

// 用工厂模式的成员函数来代替构造函数
// template <class Msg, class... Ts> auto makeMsg(Ts &&... ts) {
//   return std::make_shared<MsgImpl<Msg>>(std::forward<Ts>(ts)...);
// }

struct MsgFactoryBase {
  virtual MsgBase::Ptr create() = 0;
  virtual ~MsgFactoryBase() = default;
  using Ptr = std::shared_ptr<MsgFactoryBase>;
};

template <class Msg> struct MsgFactoryImpl : MsgFactoryBase {
  MsgBase::Ptr create() override { return std::make_shared<MsgImpl<Msg>>(); }
};

template <class Msg> auto makeFactory() {
  return std::make_shared<MsgFactoryImpl<Msg>>();
}

std::vector<MsgBase *> msgs;

int main() {
  msgs.push_back(new MsgImpl<MoveMsg>{5, 10});
  msgs.push_back(new MsgImpl<JumpMsg>{20});
  msgs.push_back(new MsgImpl<SleepMsg>{3});

  for (auto &msg : msgs) {
    msg->speak();
    msg->happy();
  }

  return 0;
}
