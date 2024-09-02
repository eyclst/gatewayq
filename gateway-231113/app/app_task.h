#if !defined(__APP_TASK_H__)
#define __APP_TASK_H__

typedef enum TaskType
{
    SEND_TYPE = 1,
    RECV_TYPE
} TaskType;

int app_task_init(const char *pathname);

int app_task_registerTask(TaskType, void (*callback)(void *), void *argv);

void app_task_wait();

int app_task_close();

#endif // __APP_TASK_H__
