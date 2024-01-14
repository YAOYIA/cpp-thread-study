#include <iostream>
#include <thread>
#include <functional>
class Hello {
public:
  Hello() {
    std::thread t(std::bind(&Hello::Entry, this, "World"));
    t.join();
  }

private:
  // 线程函数
  void Entry(const char* what) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Hello, " << what << "!" << std::endl;
  }
};

int main() {
  Hello hello;
  return 0;
}