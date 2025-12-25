#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

// 最大输入长度
#define MAX_INPUT_LEN 1024
#define MAX_ARGS 64
#define MAX_PATH 256

// 添加全局变量声明
extern pid_t fg_pid;  // Debug 这里需要全局变量来存储前台进程ID

// 命令结构体
typedef struct {
    char *args[MAX_ARGS];      // 命令参数
    char *input_file;          // 输入重定向文件
    char *output_file;         // 输出重定向文件
    int background;            // 是否后台运行
    int append_mode;           // 追加模式 (>>)
    int argc;                  // 参数个数
} Command;

// 函数声明

// 主循环函数
void shell_loop(FILE *input_stream, int interactive);

// 命令解析函数
Command* parse_command(char *line);
void free_command(Command *cmd);

// 内置命令函数
int execute_builtin(Command *cmd);
int execute_cd(Command *cmd);
int execute_clr();
int execute_dir(Command *cmd);
int execute_environ();
int execute_echo(Command *cmd);
int execute_help();
int execute_pause();
int execute_quit();

// 外部命令执行
int execute_external(Command *cmd);

// 重定向处理
void setup_redirection(Command *cmd);

// 工具函数
char* get_prompt();
void trim_whitespace(char *str);
void expand_tilde(char *path);
void set_shell_env();
void print_error(const char *msg);

// 信号处理
void setup_signal_handlers();

#endif // MYSHELL_H