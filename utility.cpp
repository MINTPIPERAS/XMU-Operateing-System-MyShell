#include "myshell.h"

// 移除字符串首尾的空白字符
void trim_whitespace(char *str) {
    int i, j;
    int len = strlen(str);
    
    // 移除开头的空白字符
    for (i = 0; i < len && isspace(str[i]); i++);
    
    if (i > 0) {
        for (j = 0; j < len - i + 1; j++) {
            str[j] = str[j + i];
        }
    }
    
    // 移除结尾的空白字符
    len = strlen(str);
    for (i = len - 1; i >= 0 && isspace(str[i]); i--) {
        str[i] = '\0';
    }
}

// 扩展波浪号路径
void expand_tilde(char *path) {
    if (path[0] == '~') {
        char expanded[MAX_PATH];
        const char *home = getenv("HOME");
        if (home == NULL) {
            home = getenv("USERPROFILE");
        }
        if (home == NULL) {
            return;  // 无法扩展
        }
        
        snprintf(expanded, sizeof(expanded), "%s%s", home, path + 1);
        strcpy(path, expanded);
    }
}

// 解析命令
Command* parse_command(char *line) {
    Command *cmd = malloc(sizeof(Command));
    if (cmd == NULL) {
        perror("malloc");
        return NULL;
    }
    
    // 初始化
    memset(cmd, 0, sizeof(Command));
    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->background = 0;
    cmd->append_mode = 0;
    cmd->argc = 0;
    
    // 复制输入以便分割
    char *input = strdup(line);
    if (input == NULL) {
        free(cmd);
        perror("strdup");
        return NULL;
    }
    
    char *saveptr;
    char *token = strtok_r(input, " \t", &saveptr);
    
    while (token != NULL) {
        // 检查特殊字符
        if (strcmp(token, "<") == 0) {
            // 输入重定向
            token = strtok_r(NULL, " \t", &saveptr);
            if (token == NULL) {
                fprintf(stderr, "myshell: 语法错误: 期望输入文件\n");
                free(input);
                free_command(cmd);
                return NULL;
            }
            cmd->input_file = strdup(token);
        }
        else if (strcmp(token, ">") == 0) {
            // 输出重定向 (覆盖)
            token = strtok_r(NULL, " \t", &saveptr);
            if (token == NULL) {
                fprintf(stderr, "myshell: 语法错误: 期望输出文件\n");
                free(input);
                free_command(cmd);
                return NULL;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = 0;
        }
        else if (strcmp(token, ">>") == 0) {
            // 输出重定向 (追加)
            token = strtok_r(NULL, " \t", &saveptr);
            if (token == NULL) {
                fprintf(stderr, "myshell: 语法错误: 期望输出文件\n");
                free(input);
                free_command(cmd);
                return NULL;
            }
            cmd->output_file = strdup(token);
            cmd->append_mode = 1;
        }
        else if (strcmp(token, "&") == 0) {
            // 后台执行
            cmd->background = 1;
        }
        else {
            // 普通参数
            if (cmd->argc < MAX_ARGS - 1) {
                cmd->args[cmd->argc] = strdup(token);
                cmd->argc++;
            } else {
                fprintf(stderr, "myshell: 参数过多\n");
                free(input);
                free_command(cmd);
                return NULL;
            }
        }
        
        token = strtok_r(NULL, " \t", &saveptr);
    }
    
    free(input);
    cmd->args[cmd->argc] = NULL;  // execvp需要以NULL结尾
    
    return cmd;
}

// 释放命令结构
void free_command(Command *cmd) {
    if (cmd == NULL) return;
    
    for (int i = 0; i < cmd->argc; i++) {
        free(cmd->args[i]);
    }
    
    if (cmd->input_file) free(cmd->input_file);
    if (cmd->output_file) free(cmd->output_file);
    
    free(cmd);
}

// 执行内置命令
int execute_builtin(Command *cmd) {
    if (cmd->argc == 0) return 0;
    
    char *command = cmd->args[0];
    
    if (strcmp(command, "cd") == 0) {
        return execute_cd(cmd);
    }
    else if (strcmp(command, "clr") == 0) {
        return execute_clr();
    }
    else if (strcmp(command, "dir") == 0) {
        return execute_dir(cmd);
    }
    else if (strcmp(command, "environ") == 0) {
        return execute_environ();
    }
    else if (strcmp(command, "echo") == 0) {
        return execute_echo(cmd);
    }
    else if (strcmp(command, "help") == 0) {
        return execute_help();
    }
    else if (strcmp(command, "pause") == 0) {
        return execute_pause();
    }
    else if (strcmp(command, "quit") == 0) {
        return execute_quit();
    }
    
    return 0;  // 不是内置命令
}

// cd命令实现
int execute_cd(Command *cmd) {
    char *path;
    
    if (cmd->argc == 1) {
        // 没有参数，显示当前目录
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s\n", cwd);
        } else {
            perror("getcwd");
        }
        return 1;
    }
    else if (cmd->argc == 2) {
        path = cmd->args[1];
        expand_tilde(path);
        
        if (chdir(path) != 0) {
            fprintf(stderr, "myshell: cd: %s: %s\n", path, strerror(errno));
        } else {
            // 更新PWD环境变量
            char cwd[MAX_PATH];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                setenv("PWD", cwd, 1);
            }
        }
        return 1;
    }
    else {
        fprintf(stderr, "myshell: cd: 参数过多\n");
        return 1;
    }
}

// clr命令实现 (清屏)
int execute_clr() {
    printf("\033[2J\033[H");  // ANSI转义序列清屏
    return 1;
}

// dir命令实现
int execute_dir(Command *cmd) {
    char *path = ".";
    FILE *output = stdout;
    int use_redirection = 0;
    
    // 检查是否指定目录
    if (cmd->argc > 1) {
        path = cmd->args[1];
        expand_tilde(path);
    }
    
    // 检查输出重定向
    if (cmd->output_file) {
        if (cmd->append_mode) {
            output = fopen(cmd->output_file, "a");
        } else {
            output = fopen(cmd->output_file, "w");
        }
        
        if (output == NULL) {
            fprintf(stderr, "myshell: 无法打开文件 '%s'\n", cmd->output_file);
            return 1;
        }
        use_redirection = 1;
    }
    
    DIR *dir = opendir(path);
    if (dir == NULL) {
        fprintf(stderr, "myshell: dir: 无法打开目录 '%s': %s\n", path, strerror(errno));
        if (use_redirection) fclose(output);
        return 1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过隐藏文件（以.开头）
        if (entry->d_name[0] != '.') {
            fprintf(output, "%s\n", entry->d_name);
        }
    }
    
    closedir(dir);
    if (use_redirection) fclose(output);
    
    return 1;
}

// environ命令实现
int execute_environ() {
    extern char **environ;
    
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    
    return 1;
}

// echo命令实现
int execute_echo(Command *cmd) {
    FILE *output = stdout;
    int use_redirection = 0;
    
    // 检查输出重定向
    if (cmd->output_file) {
        if (cmd->append_mode) {
            output = fopen(cmd->output_file, "a");
        } else {
            output = fopen(cmd->output_file, "w");
        }
        
        if (output == NULL) {
            fprintf(stderr, "myshell: 无法打开文件 '%s'\n", cmd->output_file);
            return 1;
        }
        use_redirection = 1;
    }
    
    // 打印所有参数（以单个空格分隔）
    for (int i = 1; i < cmd->argc; i++) {
        if (i > 1) fprintf(output, " ");
        fprintf(output, "%s", cmd->args[i]);
    }
    fprintf(output, "\n");
    
    if (use_redirection) fclose(output);
    
    return 1;
}

// help命令实现
int execute_help() {
    // 显示帮助手册
    const char *help_text = 
        "myshell 用户手册\n"
        "===============\n"
        "\n"
        "内置命令:\n"
        "  cd [directory]      - 更改当前目录\n"
        "  clr                 - 清屏\n"
        "  dir [directory]     - 列出目录内容\n"
        "  environ             - 显示所有环境变量\n"
        "  echo [text]         - 显示文本\n"
        "  help                - 显示此帮助信息\n"
        "  pause               - 暂停shell直到按回车\n"
        "  quit                - 退出shell\n"
        "\n"
        "I/O 重定向:\n"
        "  command < input     - 从文件读取输入\n"
        "  command > output    - 输出到文件（覆盖）\n"
        "  command >> output   - 输出到文件（追加）\n"
        "\n"
        "后台执行:\n"
        "  command &           - 在后台运行命令\n"
        "\n"
        "示例:\n"
        "  ls -la              - 列出当前目录所有文件\n"
        "  grep pattern < file - 从文件搜索模式\n"
        "  ls > listing.txt    - 将输出保存到文件\n"
        "  sleep 10 &          - 在后台休眠10秒\n";
    
    // 使用more命令过滤
    FILE *pipe = popen("more", "w");
    if (pipe == NULL) {
        printf("%s", help_text);
    } else {
        fputs(help_text, pipe);
        pclose(pipe);
    }
    
    return 1;
}

// pause命令实现
int execute_pause() {
    printf("myshell: 已暂停，按回车键继续...\n");
    getchar();  // 等待回车
    return 1;
}

// quit命令实现
int execute_quit() {
    return 1;  // 在主循环中处理退出
}

// 设置重定向
void setup_redirection(Command *cmd) {
    if (cmd->input_file) {
        int fd = open(cmd->input_file, O_RDONLY);
        if (fd < 0) {
            perror("open input file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }
    
    if (cmd->output_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= (cmd->append_mode) ? O_APPEND : O_TRUNC;
        
        int fd = open(cmd->output_file, flags, 0644);
        if (fd < 0) {
            perror("open output file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

// 执行外部命令
int execute_external(Command *cmd) {
    pid_t pid = fork();
    
    if (pid < 0) {
        // fork失败
        perror("fork");
        return 0;
    }
    else if (pid == 0) {
        // 子进程
        // 设置重定向
        setup_redirection(cmd);
        
        // 执行命令
        execvp(cmd->args[0], cmd->args);
        
        // 如果execvp返回，说明出错了
        fprintf(stderr, "myshell: %s: 命令未找到\n", cmd->args[0]);
        exit(EXIT_FAILURE);
    }
    else {
        // 父进程
        if (cmd->background) {
            // 后台进程
            printf("[%d] %s\n", pid, cmd->args[0]);
        } else {
            // 前台进程
            fg_pid = pid;
            int status;
            waitpid(pid, &status, 0);
            fg_pid = -1;
            
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                // 命令执行失败
                return 0;
            }
        }
    }
    
    return 1;
}