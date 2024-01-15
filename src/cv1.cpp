#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>


//两个线程共享的全局变量
std::mutex mutex;
std::condition_variable cv;
std::string data;
bool ready=false; //shared variable
bool processed = false;//sahred variable

void Worker(){
    std::unique_lock<std::mutex> lock(mutex);

    //wait until main thread sent data
    cv.wait(lock,[]{return ready;});

    //after wait,we own the lock
    std::cout<<"Worker thread is processing data ....."<<std::endl;
    //sleep 1 second to simulate data processing
    std::this_thread::sleep_for(std::chrono::seconds(1));
    data+="after processing";

    //send data back to the main thread
    processed = true;
    std::cout<<"Worker thread signals data processing completed."<<std::endl;
    //second 
    lock.unlock();
    cv.notify_one();
}

int main(int argc, char const *argv[])
{
    std::thread worker(Worker);
    //send data to the worker thread
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout<<"mian thread is preparing data..."<<std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));

        data = "example data";
        ready=true;
        std::cout<<"Main thread signals data ready for processing."<<std::endl;

    } 
    cv.notify_one();
    
    //wait for the worker thread to process data.
    {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock,[] {return processed;});
    }
    std::cout << "Back in main thread, data = " << data << std::endl;

    worker.join();

    return 0;
}

