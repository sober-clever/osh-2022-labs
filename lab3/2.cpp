#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>

struct Pipe {
    int fd_send;
    int fd_recv;
};

int main(int argc, char **argv) {
    int port = atoi(argv[1]);
    int fd_server;
    if ((fd_server = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd_server, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind");
        return 1;
    }
    if (listen(fd_server, 32)) {
        perror("listen");
        return 1;
    }
    fcntl(fd_server, F_SETFL, fcntl(fd_server, F_GETFL, 0) | O_NONBLOCK); 
    int fd[35], in_use[35]={0}, fd1;
    int num_of_clients=0;
       
    fd_set clients;

    while (1) {
        FD_ZERO(&clients);
        FD_SET(fd_server, &clients);
        int fdm = fd_server;
        for(int j=0; j< 32; j++){
            if(in_use[j]){
                fdm = (fdm > fd[j])? fdm : fd[j];
                fcntl(fd[j], F_SETFL, fcntl(fd[j], F_GETFL, 0) | O_NONBLOCK);
                FD_SET(fd[j], &clients);
            }
        }
        
        if (select(fdm+1, &clients, NULL, NULL, NULL) > 0) { // 找出接收到信号的套接字
            int flag = -1;
            char buffer[1024] = "";
            for(int i=0; i<32; i++){
                if(!in_use[i]) continue;
                if(FD_ISSET(fd[i], &clients)){ //说明fd[i]收到了消息
                    //printf("%d accepts new message.\n", fd[i]);
                    ssize_t len = recv(fd[i], buffer, 1000, 0);
                    if(len <=0 ){ // 表明该 fd 退出
                        flag = i;
                        break;
                    }
                    do{
                        //printf("%ld\n", len);
                        int prev = 0;
                        for(int k=0; k<len; k++){
                            if(buffer[k] == '\n'){
                                char single_message[1024] = "Message: ";
                                int s = 9;
                                for(int l= prev; l<=k; l++){
                                    single_message[s] = buffer[l];
                                    s++;
                                }
                                prev = k+1;
                                single_message[s] = '\0';
                                for(int j=0; j<32; j++){
                                    if(i==j || !in_use[j]) continue;
                                    send(fd[j], single_message, s, 0);
                                }
                            }
                        }
                    }while( (len = recv(fd[i], buffer, 1000, 0)) > 0 );
                }
            }
            if(flag != -1){
                printf("%d exited.\n", fd[flag]);
                in_use[flag] = 0;
                num_of_clients--;
                //break;
            }
            if(FD_ISSET(fd_server, &clients)){ //表明接收到客户端的连接
                int fd1 = accept(fd_server, NULL, NULL);
                if(fd1 == -1){
                    perror("accept");
                    break;
                }
                printf("Socket %d connected.\n", fd1);
                // 非阻塞I/O使我们的操作要么成功，要么立即返回错误，不被阻塞
                //fcntl(fd1, F_SETFL, fcntl(fd1, F_GETFL, 0) | O_NONBLOCK); // 将客户端的套接字设置成非阻塞
                int i;
                for(i=0; i<32; i++){
                    if(!in_use[i]) break;
                }
                if(i < 32){ //聊天室未满
                    fd[i] = fd1;
                    //printf("num: %d\n",i);
                    in_use[i] = 1;
                    //FD_SET(fd1, &clients);
                }   
            }
        } 
        else {
            break;
        }
    }
    
    return 0;
}
