#include <iostream>
#include <thread>


void printMessage(){
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout<<"Hello thread!"<<std::endl;
}

int main(int argc, char const *argv[])
{
    //create a thread
    std::thread myThread(&printMessage);

    //等待线程结束，否则线没执行完，主程序就已经结束了
    myThread.join();
    return 0;
}
