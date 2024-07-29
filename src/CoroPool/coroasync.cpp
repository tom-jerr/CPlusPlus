#include "CoroPool/coroasync.hpp"
#include <iostream>
// 实现CoAwaiter的await_suspend方法
void CoroTask::CoAwaiter::await_suspend(CoroTask::Handle hdl) const noexcept {
  newHdl.promise().contHdl = hdl;
  hdl.promise().poolPtr->runCoro(newHdl);
}
int main() {
  syncOut() << "main start on thread: " << std::this_thread::get_id()
            << std::endl;
  CoroPool pool(2);

  syncOut() << "main start task 1" << std::endl;
  CoroTask task1 = runAsync("task 1");
  pool.runTask(std::move(task1));

  syncOut() << "waitUntilNocoro" << std::endl;
  pool.waitUntilNoCoro();
  syncOut() << "main done" << std::endl;
}