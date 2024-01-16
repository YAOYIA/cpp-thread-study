#include "consumer.h"

struct out_msg_st {
    long int my_msg_type;
    int pid;
    int bread_num;
};//输出消息结构体

struct in_msg_st {
    long int my_msg_type;
    char message[512];
};//输入消息结构体

struct out_msg_st outbuf;                                       //输出消息缓冲区
struct in_msg_st inbuf;                                         //输入消息缓冲区
int msgid;                                                      //消息队列ID
sem_t* consumer_sem;                                            //消费者信号量
int exit_num;                                                   //退出等待人数

//初始化消息队列
void messageQueueInit(){
    if ((msgid = msgget((key_t)1895, IPC_CREAT|0666)) < 0) {
        perror("Could not get queue");
        exit(1);
    }
}

//生成一个在min，max之间的随机数
int getRandNum(int min, int max) {
    srand((unsigned)time(NULL));
    return (rand()%(max - min + 1) + min);
}

//获取消息队列中的消息数量
int get_message_num() {
    struct msqid_ds buf;
    msgctl(msgid, IPC_STAT, &buf);                  //获取消息队列的状态信息，将信息写入buf
    return buf.msg_qnum;
}

//该函数模拟了消费者购买面包的过程，生成购买面包数量和消费者的进程id，并通过消息队列与售货员线程进行通信，同时使用信号量进行线程同步
void buy_bread() {
    outbuf.my_msg_type = 1;
    int rand_num = getRandNum(1, 5);
    outbuf.bread_num = rand_num;
    outbuf.pid = getpid();
    printf("线程 %d 购买了 %d 个面包 \n", outbuf.pid, outbuf.bread_num);
    
    int waiting_num = get_message_num();
    if (waiting_num >= exit_num ) {
        printf("等待人数太多， 退出");
        exit(1);
    }

    sem_wait(consumer_sem);
    if (msgsnd(msgid, &outbuf, 8, 0) != 0) {                        //将消息发送到消息队列的系统调用
        perror("msg snd");
        exit(1);
    }
    printf("售货员工作，请等待\n");
    if (msgrcv(msgid, &inbuf, 512, getpid(), 0) == -1) {            //是用于从消息队列接收消息的系统调用
        perror("msg rcv");  
        exit(1);
    }
    printf("拿到货物 %s\n", inbuf.message);

    sem_post(consumer_sem);
}

//通过共享内存获取售货员线程的数量，以判断等待人数是否过多而退出
void get_saler_thread_num() {
    int fd = shm_open(SALER_SHARED_NAME, O_RDONLY, 0);
    if (fd == -1) {
        perror("shm open error");
    }
    int value;
    read(fd, &value, sizeof(value));
    close(fd);
    exit_num = 3*value;
}

//初始化消费者信号量
void init_sem() {
    consumer_sem = sem_open(CONSUMER_SEM_NAME, 0);
    if (consumer_sem == SEM_FAILED) {
        perror("consumer sem open error\n");
    }
}

//整体的流程控制，包括消息队列的初始化，获取售货员线程数量、初始化信号量，购买面包
int main(int ac, char* av[]){
    messageQueueInit();
    get_saler_thread_num();
    init_sem();
    
    if (ac == 2) {
        for (int i = 0;i < atoi(av[1]);i++) {
            buy_bread();
        }
    } else {
        buy_bread();
    }

    return 0;
}