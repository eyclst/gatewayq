#if !defined(__DAEMON_SUBPROCESS_H__)
#define __DAEMON_SUBPROCESS_H__

#include <sys/types.h>

typedef struct SubprocessStruct
{
    pid_t pid;     // 子进程ID
    char **argv;   // 子进程argv
    char **envi;   // 子进程环境变量
} Subprocess;

int daemon_subprocess_init(Subprocess *process, const char *cmd, const char *part);

int daemon_subprocess_start(Subprocess *process);

#endif // __DAEMON_SUBPROCESS_H__
