#include "app_task.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <pthread.h>
#include "log.c/log.h"

#define MSG_LEN sizeof(Task *)

typedef struct TaskStruct
{
    void (*callback)(void *);
    void *argv;
} Task;

typedef struct TaskMessageStruct
{
    long msg_type;
    Task *task;
} TaskMessage;

static int message_queue_id = -1;
static pthread_t send_thread = 0;
static pthread_t recv_thread = 0;
static TaskType send_type = SEND_TYPE;
static TaskType recv_type = RECV_TYPE;

void *app_task_executor(void *argv)
{
    // 负责处理特定任务类型的线程
    TaskType *type_p = argv;
    log_info("Task type %d executor starting...", *type_p);

    // 声明任务结构体
    TaskMessage task_message;

    while (msgrcv(message_queue_id, &task_message, MSG_LEN, *type_p, 0) > 0)
    {
        Task *task = task_message.task;
        log_trace("Received task %p", task);
        task->callback(task->argv);
        log_trace("Task %p executed", task);
        free(task);
        log_trace("Task %p freed");
    }

    return NULL;
}

int app_task_init(const char *pathname)
{
    log_debug("Task Manager Initializing...");
    // 申请消息队列
    message_queue_id = msgget(ftok(pathname, 1), IPC_CREAT | 0666);
    if (message_queue_id < 0)
    {
        return -1;
    }

    log_trace("Task queue id %d created");

    // 启动两个后台线程执行任务
    if (pthread_create(&send_thread, NULL, app_task_executor, &send_type) < 0)
    {
        return -1;
    }
    if (pthread_create(&recv_thread, NULL, app_task_executor, &recv_type) < 0)
    {
        pthread_cancel(send_thread);
        pthread_join(send_thread, NULL);
        return -1;
    }

    return 0;
}

int app_task_registerTask(TaskType type, void (*callback)(void *), void *argv)
{
    log_trace("Regitering task %p", callback);
    TaskMessage task_message;
    task_message.msg_type = type;
    task_message.task = malloc(sizeof(Task));
    log_trace("Task pointer: %p", task_message.task);
    if (!task_message.task)
    {
        log_error("Not enough memory for task %p", callback);
        return -1;
    }
    task_message.task->callback = callback;
    task_message.task->argv = argv;

    return msgsnd(message_queue_id, &task_message, MSG_LEN, 0);
}

void app_task_wait()
{
    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);
}

int app_task_close()
{
    log_info("Task manager closing...");
    TaskMessage task_message;
    // 首先关闭两个后台线程
    pthread_cancel(send_thread);
    pthread_cancel(recv_thread);

    // 消耗队列中剩余的任务
    while (msgrcv(message_queue_id, &task_message, MSG_LEN, 0, IPC_NOWAIT) > 0)
    {
        free(task_message.task);
    }

    // 关闭消息队列
    msgctl(message_queue_id, IPC_RMID, NULL);

    return 0;
}
