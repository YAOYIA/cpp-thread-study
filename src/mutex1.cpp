#include <iostream>
#include <thread>
#include <mutex>
#include <vector>


std::mutex g_mutex;
int g_count=0;


void counter(){
    g_mutex.lock();
    int i=++g_count;
    std::cout<<"count:"<<i<<std::endl;
    g_mutex.unlock();

}

int main(int argc, char const *argv[])
{
    const std::size_t SIZE=4;
    
    //创建一个线程
    // std::thread t1(&counter);
    // std::thread t2(&counter);
    // t1.join();
    // t2.join();
    //创建一组线程
    std::vector<std::thread> v;
    v.reserve(SIZE);

    for(std::size_t i=0;i<SIZE;++i){
        v.emplace_back(&counter);
    }
    for (std::thread&  t:v)
    {
        t.join();
    }
    
    return 0;
}
