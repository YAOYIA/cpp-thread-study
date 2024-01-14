#include <iostream>
#include <thread>

class counter
{
private:
    int value_;
public:
    counter(int value):value_(value){};
    void operator()(){
        while (value_>0)
        {
            value_--;
        }
        std::cout<<std::endl;
        
    }
};

int main() {
  std::thread t1(counter(3));
  t1.join();

  std::thread t2(counter(3));
  t2.detach();

  // 等待几秒，不然 t2 根本没机会执行。
  std::this_thread::sleep_for(std::chrono::seconds(4));
  
  return 0;
}