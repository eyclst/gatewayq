#if !defined(__DAEMON_RUNNER_H__)
#define __DAEMON_RUNNER_H__

#define LOG_FILE "/var/log/atguigu-gateway.log"

void daemon_runner_run(const char *cmd);

#endif // __DAEMON_RUNNER_H__
