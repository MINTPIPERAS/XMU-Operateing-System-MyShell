#include "myshell.h"

// 全局变量
pid_t fg_pid = -1;  // 前台进程ID

// 信号处理函数
void sigint_handler(int sig) {
    (void)sig;  // 添加这行，标记sig参数为未使用 
    //debug: 加入就行了 原本不知道为什么不行（可能是Ubuntu环境的问题） 
    if (fg_pid > 0) {
        kill(fg_pid, SIGINT);
    } else {
        printf("\n");
        fflush(stdout);
    }
}

void sigtstp_handler(int sig) {
    (void)sig;  // 添加这行，标记参数为未使用
    //debug: 加入就行了 原本不知道为什么不行（可能是Ubuntu环境的问题） 
    if (fg_pid > 0) {
        kill(fg_pid, SIGTSTP);
    }
}

// 设置信号处理器
void setup_signal_handlers() {
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCHLD, SIG_IGN);  // 避免僵尸进程
}

// 主函数
int main(int argc, char *argv[]) {
    // 设置shell环境变量
    set_shell_env();
    
    // 设置信号处理器
    setup_signal_handlers();
    
    // 检查是否以批处理模式运行
    if (argc > 1) {
        // 批处理模式
        FILE *batch_file = fopen(argv[1], "r");
        if (batch_file == NULL) {
            fprintf(stderr, "myshell: 无法打开文件 '%s'\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        
        shell_loop(batch_file, 0);
        fclose(batch_file);
    } else {
        // 交互模式
        shell_loop(stdin, 1);
    }
    
    return 0;
}

// shell主循环
void shell_loop(FILE *input_stream, int interactive) {
    char line[MAX_INPUT_LEN];
    char *prompt = NULL;
    
    while (1) {
        // 显示提示符（仅交互模式）
        if (interactive) {
            prompt = get_prompt();
            printf("%s", prompt);
            free(prompt);
            fflush(stdout);
        }
        
        // 读取输入
        if (fgets(line, sizeof(line), input_stream) == NULL) {
            // EOF或读取错误
            if (feof(input_stream)) {
                if (interactive) {
                    printf("\n");
                }
                break;  // 退出shell
            } else {
                perror("fgets");
                continue;
            }
        }
        
        // 移除换行符
        line[strcspn(line, "\n")] = '\0';
        
        // 跳过空行
        if (strlen(line) == 0) {
            continue;
        }
        
        // 解析命令
        Command *cmd = parse_command(line);
        if (cmd == NULL) {
            continue;
        }
        
        // 检查是否为退出命令
        if (cmd->argc > 0 && strcmp(cmd->args[0], "quit") == 0) {
            free_command(cmd);
            break;
        }
        
        // 执行命令
        int is_builtin = execute_builtin(cmd);
        if (!is_builtin) {
            // 外部命令
            execute_external(cmd);
        }
        
        // 清理命令结构
        free_command(cmd);
    }
}

// 获取提示符
char* get_prompt() {
    char *prompt = malloc(MAX_PATH + 10);
    if (prompt == NULL) {
        perror("malloc");
        return strdup("myshell> ");
    }
    
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(prompt, MAX_PATH + 10, "myshell:%s> ", cwd);
    } else {
        strcpy(prompt, "myshell> ");
    }
    
    return prompt;
}

// 设置shell环境变量
void set_shell_env() {
    char shell_path[MAX_PATH];
    if (readlink("/proc/self/exe", shell_path, sizeof(shell_path) - 1) != -1) {
        shell_path[sizeof(shell_path) - 1] = '\0';
        setenv("shell", shell_path, 1);
    } else {
        // 如果无法获取路径，使用默认值
        setenv("shell", "/bin/myshell", 1);
    }
    
    // 设置其他环境变量
    setenv("SHELL", getenv("shell"), 1);
}

// 打印错误信息
void print_error(const char *msg) {
    fprintf(stderr, "myshell: %s\n", msg);
}