#include "producer.h"


int producer_conf, producer_log;                            //配置文件和日志文件的文件描述符
int pthreadNum;                                             //当前线程的数量
time_t cur;                                                 //时间戳
sem_t * full_sem, * empty_sem, * mutex_sem, * reproduce_sem;//信号量,定义四个信号两指针，用于控制并同步线程之间的访问

//该函数是面包师傅线程的执行函数，模拟了面包的生产过程。通过信号量进行线程的同步，生产面包信息写入日志文件
void * do_produce(void * args) {
    int curStorage;                                         //存储当前仓库中面包的数量
    while (1) {
        // sem_getvalue(full_sem, &curStorage);
        // curStorage += 1;
        // //if (curStorage > 200) break;                        //
        // if (curStorage > 50) break;                           //当生产面包的数量大于50的时候停止生产
        sem_wait(empty_sem);                                  //等待信号量，实现对仓库空间的访问
        sem_wait(mutex_sem);                                  //等待信号量，实现互斥访问

        sem_getvalue(full_sem, &curStorage);
        curStorage += 1;
        //if (curStorage > 200) break;                        //
        if (curStorage > 50) break;                           //当生产面包的数量大于50的时候停止生产
        char buf[1024];                                         //定义一个字符数组，用于存储日志信息，获取当前时间戳
        time(&cur);                                             //获取当前时间戳                                             
        sprintf(buf, "面包师线程号: %d 时间: %s 产品编号: %d \n", (int)pthread_self(), asctime(localtime(&cur)), curStorage);
        write(producer_log, buf, strlen(buf));

        sem_post(mutex_sem);                                    //表示对仓库的访问结束，同时通知其他线程
        sem_post(full_sem);

    }
}


//用于读取配置文件producer.conf中的线程数量配置
int readProducerConf() {
    char buf[1024];
    int len = pread(producer_conf, buf, 2, 0);                  //IO函数，从指定文件的偏移位置读取数据
    if (len == -1) {                                            //如果成功读取到数据，返回读取的字节数
        printf("error");                                        //
    }
    return atoi(buf);                                           //Convert a string to an integer.
}

//用于创建生产者线程
void pthreadCreate(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_create(&tid[i], NULL, do_produce, NULL);            //线程标示符，新线程属性，新线程要执行的函数
    }
}

void pthreadJoin(pthread_t tid[],int num) {
    for (int i = 0;i < num;i++) {
        pthread_join(tid[i], NULL);                                 //等待一个指定线程的终止，第一个参数等待要终止的线程的标示符，第二个参数，用于存储线程的退出状态
    }
}


//用于将程序变成守护进程，同时关闭不必要的文件描述符和终端控制
int init_daemon() {
    int maxfd, fd;                                                  //maxfd用于存储最大文件描述符数，fd用于循环关闭文件描述符

    signal(SIGTTOU,SIG_IGN);                                        //忽略一些信号，以避免于守护进程的后台运行相关问题 SIGTTOU终端写操作
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
    signal(SIGCHLD, SIG_IGN);                                       //可让内核将僵尸进程转交给init进程处理，这样既不会产生僵尸进程、又省去了服务器进程回收子进程所占用的时间

    switch(fork()) {                                                //进行第一次的fork，父进程退出，子进程继续执行
        case -1: return -1;
        case 0: break;
        default: _exit(0);
    }
    
    // 子进程成为进程组首进程
    if (setsid() == -1) return -1;                                  //子进程成为新的会话领导者


	
    switch(fork()) {                                               //使子进程不再是进程组的领导者，从而无法获得控制终端
        case -1: return -1;
        case 0: break;
        default: _exit(0);
    }

    if (0 > chdir("/")) {
     perror("chdir error");
     exit(-1);
   }
    
    umask(0);                                                     //设置文件的权限掩码，取保文件创建时有最大的权限

    maxfd = sysconf(_SC_OPEN_MAX);                                 //记录日志信息，表示守护进程已经开始，返回只表示请求的系统配置信息的具体值，例如，返回的是每个进程允许的最大代开文件的数量

    for (fd = 0;fd < maxfd;fd++)close(fd);                          //
    syslog(LOG_INFO, "面包师开始生产");
    

    return 0;
}


//用于初始化信号量
void init_sem() {
    full_sem = sem_open(FULL_SEM_NAME, O_CREAT, 0666, 0);           //
    if (full_sem == SEM_FAILED) {
        perror("full sem open error\n");
    }

    // empty_sem = sem_open(EMPTY_SEM_NAME, O_CREAT, 0666, 200);
    empty_sem = sem_open(EMPTY_SEM_NAME, O_CREAT, 0666, 50);
    if (empty_sem == SEM_FAILED) {
        perror("empty sem open error\n");
    }
    
    mutex_sem = sem_open(MUTEX_SEM_NAME, O_CREAT, 0666, 1);         //信号量的值为1时，可以用作互斥锁，一个线程或进程可以使用sem_wait 操作将信号量的值减为 0，表示锁被占用。
                                                                    //其他线程或进程需要等待，直到持有锁的线程或进程通过 sem_post 操作释放锁，将信号量的值加回 1。
    if (mutex_sem == SEM_FAILED) {
        perror("mutex sem open error\n");
    }

    reproduce_sem = sem_open(REPRODUCE_SEM_NAME, O_CREAT, 0666, 1);
    if (reproduce_sem == SEM_FAILED) {
        perror("reproduce sem open error\n");
    }
}

//用于代开配置文件和日志文件
void init_file_open() {
    producer_conf = open("../config/producer.conf", O_RDONLY);
    if (producer_conf == -1) {
        perror("producer conf error");
    }
    producer_log = open("../log/producer.log", O_RDWR|O_APPEND);
    if (producer_log == -1) {
        perror("producer log error");
    }
}


//进行整体流程控制，包括守护进程的初始化、信号量和文件初始化，然后在循环中读取配置文件并创建相应数量的生产者线程
int main(int ac, char* av[]) {
    init_daemon();
    init_sem();
    init_file_open();

    while (1)
    {
        int curPthreadNum = readProducerConf();                         //读取配置文件获取当前线程数量
        pthread_t tid[curPthreadNum];                                   //创建线程的线程ID数

        if (curPthreadNum != pthreadNum) {                              //如果当前线程于之前的记录不同，更新并输出日志
            pthreadNum = curPthreadNum;
            syslog(LOG_NOTICE, "线程数量改变为 %d", pthreadNum);
        }
        sem_wait(reproduce_sem);                                        //表示是否要生产面包，控制面包师傅的生产
        pthreadCreate(tid, curPthreadNum);                              //创建并等待生产者线程
        pthreadJoin(tid, curPthreadNum);
    }
                                                                                
    sem_close(full_sem);                                               //表示仓库中面包的数量
    sem_close(empty_sem);                                              //表示仓库的剩余空间
    sem_close(mutex_sem);                                              //用于保护对仓库的访问，实现互斥
    sem_close(reproduce_sem);                                          //表示是否要生产面包，控制面包师傅的生产

    return 0;
}