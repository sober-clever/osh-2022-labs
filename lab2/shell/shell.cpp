// IO
#include <iostream>
// std::string
#include <string>
// std::vector
#include <vector>
// std::string 转 int
#include <sstream>
// PATH_MAX 等常量
#include <climits>
// POSIX API
#include <unistd.h>
// wait
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

#define READ_PORT 0  //读端
#define WRITE_PORT 1 //写端

//  delimiter - n. 分隔符
// 函数声明
std::vector<std::string> split(std::string s, const std::string &delimiter);
static void handler(int sig);
int exec_builtin(std::vector<std::string> args, std::string &cmd, std::vector<std::string> history, bool &repeat);
void execute(std::vector<std::string> args);



int main() {
  // 不同步 iostream 和 cstdio 的 buffer
  std::ios::sync_with_stdio(false);
  std::vector<std::string> history;
  // 用来存储读入的一行命令
  std::string cmd;
  
  signal(SIGINT, handler); // 这里的handler还存在问题

  // repeat 用来表示当前指令是重复之前的命令（!! 或 !n），还是执行输入的命令
  bool  repeat = false;
  
  while (true) {

    // repeat = false，表示执行的命令来自用户输入 
    if(!repeat){
        // 打印提示符
        std::cout << "\033[32mshell\033[0m#";

        // 读入一行。std::getline 结果不包含换行符。
        std::getline(std::cin, cmd);
        //getchar();
        history.push_back(cmd);
    }
    else{ //表示运行的是之前的指令（通过 !n 或者 !!）
        repeat = false;
    }
    
    // 检查是否含有管道
    std::vector<std::string> cmds = split(cmd, "|");
    //std::cout<<"cmds len:"<<cmds.size()<<"\n";
    // 单条指令 或者 空指令
    if(cmds.size() <= 1){
      // 按空格分割命令为单词
      //  ags 是所有单词组成的向量
      std::vector<std::string> args = split(cmd, " ");  

      // 没有可处理的命令
      if (args.empty()) {
        continue;
      }

      // 执行内建指令，传 cmd 和 history 以执行 !!、!n 与 history 指令
      if(exec_builtin(args, cmd, history, repeat)!=-1)
        continue;

      // 外部命令
      execute(args);
    }
    // 处理管道的情况
    else{
      int read_fd; //上一管道的读端口
      int len = cmds.size();
      // 在下面的循环中会创建 len - 1 个管道
      // 注意，Linux 管道中的各条指令时并行的
      for(int i = 0; i < len; i++){
        std::string single_cmd = cmds[i]; //取出单条指令
        //std::cout<<single_cmd<<"\n";
        int fd[2];
        if(i != len-1){
          int ret = pipe(fd); //创建管道
          if(ret == -1){
            std::cout<<"Pipe creation failed.\n";
            continue;
          }
        }
        
        pid_t pid = fork();
        if(pid == 0){
          // 子进程
          if(i < len - 1){ //除了最后一条指令，其余指令都要将结果输入到管道中
            close(fd[READ_PORT]);
            dup2(fd[WRITE_PORT], STDOUT_FILENO); 
            //把标准输出重定向到管道的写端，这时进程（指令）的输出结果不会输出到屏幕上，
            //而是会输出到管道里
            close(fd[WRITE_PORT]);
          }
          if(i > 0){ // 除了第一条指令，都要从管道读取信息
            close(fd[WRITE_PORT]);
            dup2(read_fd, STDIN_FILENO); 
            //把标准输入重定向到上一个子进程和父进程通信管道的读端，读取上一条指令的输出结果
            //这时进程（指令）的输入不会来自键盘，而是从管道的读端读取
            close(fd[READ_PORT]);
          }
          std::vector<std::string> args = split(single_cmd, " ");
          if(exec_builtin(args, cmd, history, repeat) == -1)
            execute(args);
          exit(255);
        }

        // 父进程
        if(i>0) close(read_fd); // 父进程
        if(i<len-1) read_fd = fd[READ_PORT];  //保留下前一个
        // std::cout<<read_fd<<"\n";
        close(fd[WRITE_PORT]); 
        // 父进程的写端一定要关闭，否则管道会被阻塞
        //（只有管道所有的写端口都被关闭时，管道才不会被阻塞，
        // 否则就会阻塞直到写端有内容写入，类似于 scanf 等待用户输入）
      }
      // 如果该进程没有子进程，则wait函数返回 -1
      // 如果该进程有子进程，则wait函数返回最近运行结束的子进程的id
      // 下面的 while 循环确保了所有子进程都结束运行后，才会退出去执行下一条指令
      // （只有 wait 返回值为 0，才能确保子进程都执行完毕）
      while(wait(nullptr) > 0) ;
    }
  }
}

// 经典的 cpp string split 实现
// https://stackoverflow.com/a/14266139/11691878
std::vector<std::string> split(std::string s, const std::string &delimiter) {
  std::vector<std::string> res;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {  // 在s中找到了分界符
    token = s.substr(0, pos);
    res.push_back(token);
    s = s.substr(pos + delimiter.length());
  }
  res.push_back(s);
  return res;
}


static void handler(int sig){
  int status;
  pid_t pid = waitpid(0, &status, WNOHANG);
  if(sig==SIGINT){
    // pid == -1 表明 waitpid 异常
    if(pid != -1){
      std::cout<<"\n";
      //fflush(stdout);
      //fprintf(stderr, "#\n");
      //std::cout<<"pid != -1\n";
      return ;
    }
    else{ 
      write(2, "\n\033[32mshell\033[0m#", 17);
      //std::cout<<"\n#";
      //std::cout<<"pid == -1\n";
      return ;
    }
  }
}

  
// 执行内建指令
int exec_builtin(std::vector<std::string> args, std::string &cmd, std::vector<std::string> history, bool &repeat){
  // 更改工作目录为目标目录
    if (args[0] == "cd") {
      if (args.size() <= 1) {
        // 输出的信息尽量为英文，非英文输出（其实是非 ASCII 输出）在没有特别配置的情况下（特别是 Windows 下）会乱码
        // 如感兴趣可以自行搜索 GBK Unicode UTF-8 Codepage UTF-16 等进行学习
        std::cout << "Insufficient arguments\n";
        // 不要用 std::endl，std::endl = "\n" + fflush(stdout)
        return 1;
      }

      // 调用系统 API
      int ret = chdir(args[1].c_str());
      if (ret < 0) {
        std::cout << "cd failed\n";
      }
      return 1;
    }

    // 显示当前工作目录
    if (args[0] == "pwd") {
      std::string cwd;

      // 预先分配好空间
      cwd.resize(PATH_MAX);

      // std::string to char *: &s[0]（C++17 以上可以用 s.data()）
      // std::string 保证其内存是连续的
      const char *ret = getcwd(&cwd[0], PATH_MAX);
      if (ret == nullptr) {
        std::cout << "cwd failed\n";
      } else {
        std::cout << ret << "\n";
      }
      return 1;
    }

    // 设置环境变量
    if (args[0] == "export") {
      for (auto i = ++args.begin(); i != args.end(); i++) { //遍历args的所有参数
        std::string key = *i;

        // std::string 默认为空
        std::string value;

        // std::string::npos = std::string end
        // std::string 不是 nullptr 结尾的，但确实会有一个结尾字符 npos
        size_t pos;
        if ((pos = i->find('=')) != std::string::npos) {
          key = i->substr(0, pos);  //环境变量的名称
          value = i->substr(pos + 1);   //环境变量的值
        }

        // .c_str 用于把 std::string 类型转化成C语言能理解的 char*
        int ret = setenv(key.c_str(), value.c_str(), 1);
        if (ret < 0) {
          std::cout << "export failed\n";
        }
      }
      return 1;
    }

    // 退出
    if (args[0] == "exit") {
      if (args.size() <= 1) {
        exit(0); //直接退出程序
      }

      // std::string 转 int
      std::stringstream code_stream(args[1]);
      int code = 0;
      code_stream >> code;

      // 转换失败
      if (!code_stream.eof() || code_stream.fail()) {
        std::cout << "Invalid exit code\n";
        return 1;
      }

      return code;
    }

    // history n指令
    if(args[0] == "history" && args.size() > 1){
        int n = atoi(args[1].c_str());
        for(auto i = 0; i < history.size() && i < n; i++){
            std::cout<<i+1<<" "<<history[i]<<"\n";
        }
        return 1;
    }

    //!n 指令
    if(args[0][0] == '!' && args[0]!="!!"){
        int n = atoi(args[0].substr(1).c_str()); // 获得n
        if(0 < n && n <= history.size()){
            cmd = history[n-1];
            std::cout<<cmd<<"\n";
            repeat = true;
        }
        else{
            std::cout<<"bash: !"<<n<<": event not found"<<"\n";
        }
        return 1;
    }

    // !! 指令
    if(args[0] == "!!" && args.size()==1){
        int index = history.size()-2;
        while(history[index] == "!!" && index>=0){
            index--;
        }
        if(index>=0){
            cmd = history[index];
            std::cout<<cmd<<"\n";
            repeat = true;
        }
        return 1;
    }

    return -1; //前面的都没有执行，说明是外部指令
}

// 执行外部指令
void execute(std::vector<std::string> args){
  // 创建子进程以执行外部命令
  pid_t pid = fork(); 

  // std::vector<std::string> 转 char **
  char *arg_ptrs[args.size() + 1];
  for (auto i = 0; i < args.size(); i++) {
    arg_ptrs[i] = &args[i][0];  // args[i]的类型是 std::string
  }

  // exec p 系列的 argv 需要以 nullptr 结尾
  arg_ptrs[args.size()] = nullptr;

  if (pid == 0) {
    // 这里只有子进程才会进入
    // execvp 会完全更换子进程接下来的代码，所以正常情况下 execvp 之后这里的代码就没意义了
    // 如果 execvp 之后的代码被运行了，那就是 execvp 出问题了
    // execvp 会从 PATH 环境变量所指的目录中查找符合参数 file 的文件名，
    // 找到后便执行该文件，然后将第二个参数 argv 传给该欲执行的文件。
    execvp(args[0].c_str(), arg_ptrs);

    // 所以这里直接报错
    exit(255);
  }

  // 这里只有父进程（原进程）才会进入
  // 父进程等待子进程完成
  // 如果成功，wait会返回被收集的子进程的进程ID，如果调用进程没有子进程，调用就会失败，此时wait返回-1
  int ret = wait(nullptr);
  if (ret < 0) {
    std::cout << "wait failed";
  }
}

