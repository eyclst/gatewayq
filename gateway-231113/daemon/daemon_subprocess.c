#include "daemon_subprocess.h"
#include <stdlib.h>
#include <unistd.h>
#include <log.c/log.h>

extern char **environ;

int daemon_subprocess_init(Subprocess *process, const char *cmd, const char *part)
{
    process->pid = -1;
    // 初始化启动命令
    process->argv = malloc(3 * sizeof(char *));
    if (!process->argv)
    {
        return -1;
    }

    process->argv[0] = (char *)cmd;
    process->argv[1] = (char *)part;
    process->argv[2] = NULL;

    // 初始化环境变量
    process->envi = environ;

    return 0;
}

int daemon_subprocess_start(Subprocess *process)
{
    // fork
    log_info("Subprocess %s starting...", process->argv[1]);
    process->pid = fork();
    if (process->pid < 0)
    {
        return -1;
    }

    if (process->pid > 0)
    {
        return 0;
    }
    // execve
    if (execve(process->argv[0], process->argv, process->envi))
    {
        log_fatal("Subprocess %s start failed", process->argv[1]);
        exit(EXIT_FAILURE);
    }
    return 0;
}
