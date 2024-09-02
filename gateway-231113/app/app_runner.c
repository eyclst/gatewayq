#include "app_runner.h"
#include <signal.h>
#include "app_task.h"
#include "app_serial.h"
#include "app_router.h"
#include "log.c/log.h"

static SerialDevice device0;

static void app_runner_signalHandler(int signal)
{
    if (signal == SIGTERM || signal == SIGINT)
    {
        // 执行关闭逻辑
        app_task_close();
        app_router_close();
    }
}

static int app_runner_init(const char *filename)
{
    int result = 0;
    // 注册结束信号
    signal(SIGTERM, app_runner_signalHandler);
    signal(SIGINT, app_runner_signalHandler);

    // 初始化各种组件
    result = app_serial_init(&device0, "/dev/ttyS2", BAUD_115200);
    log_warn("Serial init result: %d", result);
    result = app_serial_setConnectionType(&device0, BLUETOOTH);
    log_warn("BT set result: %d", result);

    result = app_router_init();
    log_warn("Router init result: %d", result);
    result = app_router_registerDevice((Device *)&device0);
    log_warn("Device register result: %d", result);

    result = app_task_init(filename);
    log_warn("Task manager result: %d", result);
    return 0;
}

void app_runner_run(const char *filename)
{
    // 首先初始化
    app_runner_init(filename);

    // 卡住主线程
    app_task_wait();
}