#include <saler.h>
/*
面包售货系统
*/
struct in_msg_st {
    long int my_msg_type;
    int pid;
    int bread_num;
};//输入消息结构体定义

struct out_msg_st {
    long int my_msg_type;
    char message[512];
};//输出消息结构体定义

int msgid;                                      //消息队列id
int saler_log;                                  //文件描述符，用于写入售货员日志
int th_num;                                     //售货员线程数量
struct in_msg_st inbuf;                         //输入消息缓冲区
struct out_msg_st outbuf;                       //输出消息缓冲区
time_t cur;                                     //时间戳
pthread_mutex_t lock;                           //互斥锁
sem_t* consumer_sem, * full_sem, * empty_sem, * mutex_sem, * reproduce_sem;//信号量

int getRandNum(int min, int max) {
    srand((unsigned)time(NULL));
    return (rand()%(max - min + 1) + min);
}                                               //生成在min，max之间的随机数


//售货员线程的执行函数，负责处理消息队列中的请求，更新日志，并通过信号量进行线程同步
void * do_sale(void * args) {
    while (1) {
        if (msgrcv(msgid, &inbuf, sizeof(int)*2, 1, 0) == -1) {                         //消息队列，返回值表示实际接收到的消息的长度，接受到的消息放到inbuf中
            perror("msg rcv error");
        }   
        int num = inbuf.bread_num;
        int cur_bread;

        sem_getvalue(full_sem, &cur_bread);                                             //获取面包数量

        if (cur_bread < 2 || cur_bread < num) {                                         //如果当前信号量值小于2或者小于num，则通过信号量reproduce_sem增加生产面包的数量
            sem_post(reproduce_sem);
        }
        outbuf.my_msg_type = inbuf.pid;

        
        while (num != 0) {                                                              ////循环处理售货员线程要售卖俄面包数量

            sem_wait(full_sem);                                                         //等待满的信号量，表示有面包可以售卖
            sem_wait(mutex_sem);                                                        //等待互斥量，保护共享资源

            char buf[1024];                                             
            time(&cur);
            int productId;
            sem_getvalue(full_sem, &productId);                         
            sprintf(buf, "售货员线程号: %d 时间: %s 产品编号: %d \n", (int)pthread_self(), asctime(localtime(&cur)), productId+1);
            write(saler_log, buf, strlen(buf));

            sem_post(mutex_sem);
            sem_post(empty_sem);
            
            num--;
        }

        pthread_mutex_lock(&lock);                                                      //加锁，防止多个售货员同时写入日志导致信息混乱
        char buf[1024];
        time(&cur);
        sprintf(buf, "售货员线程号: %d 时间: %s 卖面包数量: %d \n", (int)pthread_self(), asctime(localtime(&cur)), inbuf.bread_num);
        write(saler_log, buf, strlen(buf));
        pthread_mutex_unlock(&lock);

        strcpy(outbuf.message, "success");                                              //设置输出消息类型
        if (msgsnd(msgid, &outbuf, 512, IPC_NOWAIT) == -1) {                            //将消息发送到消息队列 
            perror("msg snd error");
        }
    }
}

//创建售货员线程
void pthreadCreate(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_create(&tid[i], NULL, do_sale, NULL);
    }
}

//等待售货员线程结束
void pthreadJoin(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_join(tid[i], NULL);
    }
}

//初始化信号量
void init_sem() {
    full_sem = sem_open(FULL_SEM_NAME, 0);
    if (full_sem == SEM_FAILED) {
        perror("full sem open error\n");
    }

    empty_sem = sem_open(EMPTY_SEM_NAME, 0);
    if (empty_sem == SEM_FAILED) {
        perror("empty sem open error\n");
    }

    mutex_sem = sem_open(MUTEX_SEM_NAME, 0);
    if (mutex_sem == SEM_FAILED) {
        perror("mutex sem open error\n");
    }

    reproduce_sem = sem_open(REPRODUCE_SEM_NAME, 0);
    if (reproduce_sem == SEM_FAILED) {
        perror("reproduce sem open error\n");
    }
    
    printf("%d 线程数量\n", th_num);
    consumer_sem = sem_open(CONSUMER_SEM_NAME, O_CREAT, 0666, th_num);
    if (consumer_sem == SEM_FAILED) {
        perror("consumer sem open error\n");
    }
}

//初始化消息队列
void init_msgqueue() {
    if ((msgid = msgget((key_t)1895, IPC_CREAT|0666)) < 0){
        perror("Could not create queue");
    }
}

//打开日志文件
void init_file_open() {
    saler_log = open("../log/saler.log", O_RDWR|O_APPEND);
}

//共享售后员线程数量
void share_saler_thread_num() {
    int fd = shm_open(SALER_SHARED_NAME, O_CREAT|O_RDWR, 0666);
    void * addr;
    if (fd < 0) {
        perror("shm open error");
        exit(1);
    }
    if (ftruncate(fd, sizeof(int)) == -1) {
        perror("ftruncate error");
    }
    write(fd, &th_num, sizeof(th_num));
    close(fd);
}

//main函数进行了整体流程控制，包括初始化、创建线程、等待线程结束以及清理资源等步骤
int main(int ac, char *av[]) {
    th_num = getRandNum(2, 10);                                     //生成一个随机数，用于确定售货员线程的数量
    init_sem();
    init_msgqueue();
    init_file_open();
    pthread_mutex_init(&lock, NULL);                                //初始化互斥锁。为了保护共享资源的访问
    share_saler_thread_num();                                       //共享售货员线程数量。该函数通过共享内存机制将售货员线程的数量写入一个共享的内存区域

    pthread_t tid[th_num];                                          //生明一个线程id数组，用于存储售货员线程的id
    pthreadCreate(tid, th_num);
    pthreadJoin(tid, th_num);


    //资源释放
    sem_close(full_sem);
    sem_close(empty_sem);
    sem_close(mutex_sem);
    sem_close(reproduce_sem);
    sem_close(consumer_sem);

    msgctl(msgid, IPC_RMID, NULL);                                  //删除消息队列
    
    return 0;
}