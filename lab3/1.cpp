#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <queue>
#include <string>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int num_of_clients = 0; // 用于存储连接的客户端数量
//int fd_clients[35]; // 用于存储每个客户端的接收端
int send_fd = 0; // 用于表明是哪个用户发送的消息
int in_use[35] = {0};
struct Pipe {
    int fd;
    int num; // 用于记录自身的编号
    //int fd_recv; // 可以向至多 31 个套接字发送消息
    std::queue<std::string> send_q; // 存下每个客户端的消息队列
}clients[35];

void *handle_chat(void *data) {
    
    struct Pipe *pipe = (struct Pipe *)data;
    char buffer[1024] = "";
    char single_message[1024] = "Message: ";
    ssize_t len;
    int prev=0;
    int num = pipe->num;
    // printf("Client %d is using.\n", num);
    //send_fd = pipe->num;
    while ((len = recv(pipe->fd, buffer, 1000, 0)) > 0) {
        //printf("Sockect %d sending the length %ld\n", pipe->fd, len);
        //pthread_mutex_lock(&mutex);
        prev = 0;
        for(int i = 0; i < len; i++){
            if(buffer[i]=='\n'){
                int k = 9;
                for(int j = prev; j <= i; j++){
                    single_message[k] = buffer[j];
                    k++;
                }
                prev = i + 1;
                single_message[k]='\0';
                //printf("%s", single_message);
                std::string me = single_message;
                pthread_mutex_lock(&mutex);
                for(int n=0; n<32; n++){
                     // n 为要发送的目标， num 为发送消息的节点

                    // 不能发给自己，不能发给空闲的节点
                    if(n==num || !in_use[n]) continue;

                    // 将消息加入目标节点的消息队列
                    clients[n].send_q.push(me);
                    
                    //send(clients[n].fd, single_message, k, 0);
                }
                pthread_mutex_unlock(&mutex);
                //send(pipe->fd_recv, single_message, k, 0);
            }
        }
        if(buffer[len-1]!='\n'){ //说明 buffer 中没有换行符
            int k = 9;
            std::string me;
            for(int i=prev; i < len; i++){
                single_message[k] = buffer[i];
                k++;
            }
            single_message[k] = '\0';
            me = single_message;
            pthread_mutex_lock(&mutex);
            for(int n=0; n<32; n++){
                    // n 为要发送的目标， num 为发送消息的节点

                // 不能发给自己，不能发给空闲的节点
                if(n==num || !in_use[n]) continue;

                // 将消息加入目标节点的消息队列
                clients[n].send_q.push(me);
                pthread_mutex_unlock(&mutex);
            }
        }
    }
    in_use[num] = 0;
    num_of_clients--;
    printf("Client %d exited.\n", num);
    return NULL;
}


void *send_handle(void *data){
    struct Pipe* pipe = (struct Pipe *) data;
    int num = pipe->num;
    while(1){
        if(!in_use[num]) break;
        char test[] = "1";
        //int ret = send(pipe->fd, test, 0, 0);
        //if(ret<=0) break;
        while(!clients[num].send_q.empty()){

            pthread_mutex_lock(&mutex);

            std::string me = clients[num].send_q.front();
            clients[num].send_q.pop();
            pthread_mutex_unlock(&mutex);

            send(pipe->fd, me.c_str(), me.length(), 0);
        }
    }
    //printf("Client %d exited.\n", num);
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

    // 监听套接字，同时将套接字变成被动类型，等待客户机连接，最大可容纳 32 个连接
    if (listen(fd, 32)) {
        perror("listen");
        return 1;
    }
    
    while(1){
        int fd_cli = accept(fd, NULL, NULL);
        printf("Socket %d connected.\n", fd_cli);
        if(fd_cli == -1){
            perror("accept");
            return 1;
        }
        
        // 寻找可用的编号
        int j; 
        for(j=0; j<32; j++){
            if(!in_use[j]) break;
        }
        // 没有空闲的位置
        if(j==32) continue;
        clients[j].fd = fd_cli;
        clients[j].num = j;
        in_use[j] = 1; //表明节点 j 已经被占用

        num_of_clients++;
        pthread_t cli_thread;
        pthread_create(&cli_thread, NULL, handle_chat, (void *)&clients[j]);
        pthread_t cli_send;
        pthread_create(&cli_send, NULL, send_handle, (void *)&clients[j]);
    }


    return 0;
}

