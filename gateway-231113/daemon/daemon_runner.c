#define _DEFAULT_SOURCE 1
#include "daemon_runner.h"
#include "daemon_subprocess.h"
#include <stdlib.h>
#include "log.c/log.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>

#define SUBPROCESS_COUNT 2

static Subprocess process[SUBPROCESS_COUNT];
static int running = 1;

static void daemon_runner_close(int signal)
{
    assert(signal == SIGTERM);
    // 关闭主循环
    running = 0;
}

void daemon_runner_run(const char *cmd)
{
    log_set_level(LOG_INFO);
    // 本进程后台化
    if (daemon(0, 1) < 0)
    {
        exit(EXIT_FAILURE);
    }

    // 手动关闭所有文件描述符
    close(STDERR_FILENO);
    close(STDOUT_FILENO);
    close(STDIN_FILENO);

    // 删除旧的日志
    unlink(LOG_FILE);

    // 重新打开文件描述符
    open("/dev/null", O_RDWR);
    open(LOG_FILE, O_RDWR | O_CREAT);
    open(LOG_FILE, O_RDWR | O_CREAT);

    // 注册SIGTERM信号处理
    if (signal(SIGTERM, daemon_runner_close) == SIG_ERR)
    {
        exit(EXIT_FAILURE);
    }

    // 初始化子进程
    if (daemon_subprocess_init(process, cmd, "app") < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (daemon_subprocess_init(process + 1, cmd, "ota") < 0)
    {
        exit(EXIT_FAILURE);
    }

    // 启动子进程
    for (int i = 0; i < SUBPROCESS_COUNT; i++)
    {
        if (daemon_subprocess_start(process + i) < 0)
        {
            log_fatal("Subprocess %s start failed", process->argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    // 如果子进程退出，重启子进程
    while (running)
    {
        // 每100ms检查一次
        usleep(100000);
        int subprocess_status = 0;
        // 查看一下是否有自进程退出
        int subprocess_pid = waitpid(-1, &subprocess_status, WNOHANG);

        if (subprocess_pid < 0)
        {
            exit(EXIT_FAILURE);
        }

        if (subprocess_pid == 0)
        {
            // 说明没有子进程退出
            continue;
        }

        // 找到这个子进程并重启
        for (int i = 0; i < SUBPROCESS_COUNT; i++)
        {
            if (subprocess_pid == process[i].pid)
            {
                daemon_subprocess_start(process + i);
                break;
            }
        }
    }

    // 关闭所有子进程，并回收资源
    log_info("Terminating subprocess...");
    for (int i = 0; i < SUBPROCESS_COUNT; i++)
    {
        int status = 0;
        kill(process[i].pid, SIGTERM);
        waitpid(-1, &status, 0);
    }
    log_info("All subprocess closed, exiting...");
}