#include<unistd.h>
#include<sys/wait.h>
#include<sys/ptrace.h>
#include<sys/user.h>
#include<stdlib.h>
#include<cstdio>
#include<iostream>
#include<sstream>
#include<string>

int main(int argc, char *argv[]){
    pid_t pid;
    int status;
    struct user_regs_struct regs;
    int orig_rax; 

    pid = fork();
    if(pid == 0){
        // 子进程
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execvp(argv[1], argv + 1);
        exit(0);
    }
    else{
        
        waitpid(pid, 0, 0);
        ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
        // 父进程
        while(1){
            
            // 监控子进程的系统调用
            ptrace(PTRACE_SYSCALL, pid, 0, 0); 
            
            // 该函数执行结束后子进程停在系统调用的入口
            waitpid(pid, 0, 0);

            struct user_regs_struct regs;

            // 查看系统调用的编号，以及传入系统调用函数的参数
            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            long syscall = regs.orig_rax;
            
            fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)", syscall,
            (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
            (long)regs.r10, (long)regs.r8,  (long)regs.r9
            );

            ptrace(PTRACE_SYSCALL, pid, 0, 0);
        
            // 该函数执行结束后子进程停在系统调用的出口
            waitpid(pid, 0, 0);

            ptrace(PTRACE_GETREGS, pid, 0, &regs);
            fprintf(stderr, " = %ld\n", (long)regs.rax);
            if(syscall==231) exit(0);
        }
    }

 
    return 0;
}
