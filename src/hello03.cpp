#include <iostream>
#include <thread>


class hello03
{
public:
    void operator()(const char* what){
        std::cout<<"hello"<<what<<std::endl;
    }
    hello03(/* args */);
    ~hello03();
};
int main(int argc, char const *argv[])
{
    hello03 hello;

    //方法一：拷贝函数对象
    std::thread mythread(hello,"thread");
    mythread.join();


    //方式二：不拷贝函数对象，通过boost::ref传入引用
    //用户必须保证被线程引用的函数对象，拥有超出线程的声明周期
    //
    std::thread mythread2(std::ref(hello),"world");
    mythread2.join();


    return 0;
}
