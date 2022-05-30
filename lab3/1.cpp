#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

struct Pipe {
    int fd_send;
    int fd_recv;
};

void *handle_chat(void *data) {
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[1024] = "";
    char single_message[1024] = "Message: ";
    ssize_t len;
    int prev=0;
    while ((len = recv(pipe->fd_send, buffer, 1000, 0)) > 0) {
        //printf("%ld\n", len);
        prev = 0;
        for(int i = 0; i < len; i++){
            if(buffer[i]=='\n'){
                //printf("%d\n", i);
                int k = 9;
                for(int j = prev; j <= i; j++){
                    single_message[k] = buffer[j];
                    k++;
                }
                prev = i + 1;
                single_message[k+1]='\0';
                //printf("%s", single_message);
                send(pipe->fd_recv, single_message, k, 0);
            }
            
        }
        /*
        int k = 9;
        for(int j=prev; j<len; j++){
            single_message[k] = buffer[j];
            k++;
        }
        send(pipe->fd_recv, single_message, k, 0);
        */
    }
    return NULL;
}

int main(int argc, char **argv) {
     // 套接字所在的端口号由运行时的命令行参数给定
    int port = atoi(argv[1]);
    int fd;
    // 创建套接字
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    printf("Server socket %d is created.\n", fd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    
    // 将该套接字与地址绑定，把地址族中的地址赋给套接字
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }

    // 监听套接字，同时将套接字变成被动类型，等待客户机连接
    if (listen(fd, 2)) {
        perror("listen");
        return 1;
    }

    // 创建两个连接
    int fd1 = accept(fd, NULL, NULL);
    //printf("Socket %d coonected.\n", fd1);
    int fd2 = accept(fd, NULL, NULL);
    //printf("Scoket %d connected.\n", fd2);
    if (fd1 == -1 || fd2 == -1) {
        perror("accept");
        return 1;
    }

    // 创建两个线程
    pthread_t thread1, thread2;
    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;
    pthread_create(&thread1, NULL, handle_chat, (void *)&pipe1);
    pthread_create(&thread2, NULL, handle_chat, (void *)&pipe2);
    //printf("Thread created.\n");
    // 等待两个线程运行结束
    pthread_join(thread1, NULL);
    //printf("Thread 1 exited.\n");
    pthread_join(thread2, NULL);
    //printf("Thread 2 exited.\n");
    return 0;
}

