谭骏飞 PB20061276

## 和系统`bash`中不同的部分

- 在指令（如 sleep 指令）运行期间用 Ctrl + C 中断会输出两次换行

- 不支持 !! 或者 !n 与重定向或者管道结合

- !! 或者 !n 只能单独使用

- 不支持 !! 对应的上一条指令也是 !! 或 !n 的情况，!n 同理

- history 的输出和 bash 不一样，`shell.cpp` 的 history 会记录所有输入过的指令，包括 !! 这类指令

- echo $ 只支持查询一个或多个环境变量，比如：

  ```bash
  echo $USERNAME $HOME
  ```

  但是不支持下面这种（下面这种会重新回复一遍 'aaa $USERNAME'）：

  ```bash
  echo aaa $USERNAME
  ```




## 选做部分

- 支持 history 的持久化查询，存历史记录的文件为`shell.history`存在` shell.cpp`对应的可执行文件所在的目录下
- 支持 Ctrl + D 退出，Ctrl + D 退出不会输出换行符
- 支持 `echo $`查询环境变量的值
