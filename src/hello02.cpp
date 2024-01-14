#include <iostream>
#include <thread>

void Hello(const char* what){
    std::cout<<"hello"<<what<<std::endl;
}


int main(int argc, char const *argv[])
{
    std::thread mythread(&Hello,"thread");
    mythread.join();
    return 0;
}
