#include "app_device.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include "log.c/log.h"
#include "app_task.h"

#define BUFFER_SIZE 16384

void app_device_defaultRecvTask(void *argv)
{
    log_trace("Recv task started");
    // 从buffer中读取数据，调用发送回调函数
    Device *device = argv;
    unsigned char buf[1024];
    // 首先读取出两个长度
    app_buffer_read(device->recv_buffer, buf, 2);
    app_buffer_read(device->recv_buffer, buf + 2, buf[0]);
    app_buffer_read(device->recv_buffer, buf + 2 + buf[0], buf[1]);

    // 调用发送回调函数
    device->vptr->recv_callback(device, buf, buf[0] + buf[1] + 2);
    log_trace("Recv task ended");
}

void app_device_defaultSendTask(void *argv)
{
    // 从send_buffer中读数据，写到fd
    Device *device = argv;
    int len = 0;
    unsigned char buf[1024];

    app_buffer_read(device->send_buffer, buf, 2);
    app_buffer_read(device->send_buffer, buf + 2, buf[0]);
    app_buffer_read(device->send_buffer, buf + 2 + buf[0], buf[1]);

    len = buf[0] + buf[1] + 2;
    // 如果目前设备不可写，等待
    while (app_common_getCurrentTimestamp() - device->last_write_ts < 100)
    {
        usleep(1000);
    }

    if (device->vptr->pre_send)
    {
        device->vptr->pre_send(device, buf, &len);
    }

    if (len > 0)
    {
        log_trace("Write to device");
        write(device->fd, buf, len);
    }
}

// 默认的后台线程
void *app_device_defaultBackgroundThread(void *argv)
{
    unsigned char buf[1024];
    int result = 0;
    // 从设备中读取数据并写到读取缓存，然后发送消息
    Device *device = argv;
    // 如果设备正在运行
    while (device->is_running)
    {
        // 从设备中读取数据
        result = read(device->fd, buf, 1024);
        if (result < 0)
        {
            break;
        }
        // 加一步预处理
        if (device->vptr->pre_recv)
        {
            device->vptr->pre_recv(device, buf, &result);
        }
        if (result == 0)
        {
            continue;
        }

        // 写入设备缓存(期望写入的是一条完整的消息：id长度、消息长度、id、消息)
        app_buffer_write(device->recv_buffer, buf, result);

        // 发送消息
        app_task_registerTask(RECV_TYPE, device->vptr->recv_task, device);
    }
    return NULL;
}

int app_device_init(Device *device, const char *device_name)
{
    log_info("Opening device %s", device_name);
    bzero(device, sizeof(Device));
    device->fd = open(device_name, O_RDWR | O_NOCTTY);
    if (device->fd < 0)
    {
        log_error("Device %s open fail", device_name);
        return -1;
    }

    device->device_name = device_name;

    device->type = GENERAL;
    device->connection_type = OTHER;
    device->last_write_ts = 0;
    device->recv_buffer = malloc(sizeof(Buffer));
    if (!device->recv_buffer)
    {
        log_error("Not enough memory for device %s", device_name);
        return -1;
    }

    device->send_buffer = malloc(sizeof(Buffer));
    if (!device->send_buffer)
    {
        app_device_close(device);
        log_error("Not enough memory for device %s", device_name);
        return -1;
    }

    device->vptr = malloc(sizeof(struct DeviceVTable));
    if (!device->vptr)
    {
        app_device_close(device);
        log_error("Not enough memory for device %s", device_name);
        return -1;
    }

    // 到这里为止，所有申请都成功了
    if (app_buffer_init(device->send_buffer, BUFFER_SIZE))
    {
        app_device_close(device);
        return -1;
    }
    if (app_buffer_init(device->recv_buffer, BUFFER_SIZE))
    {
        app_device_close(device);
        return -1;
    }

    // 给后台线程指针赋值
    device->vptr->device_backgound_func = app_device_defaultBackgroundThread;

    // 给设备的recv_task和send_task赋值
    device->vptr->recv_task = app_device_defaultRecvTask;
    device->vptr->send_task = app_device_defaultSendTask;

    return 0;
}

void app_device_close(Device *device)
{
    if (device->vptr)
    {
        free(device->vptr);
        device->vptr = NULL;
    }

    if (device->send_buffer)
    {
        app_buffer_close(device->send_buffer);
        free(device->send_buffer);
        device->send_buffer = NULL;
    }
    if (device->recv_buffer)
    {
        app_buffer_close(device->recv_buffer);
        free(device->recv_buffer);
        device->recv_buffer = NULL;
    }
    if (device->fd > 0)
    {
        close(device->fd);
        device->fd = -1;
    }
}

int app_device_start(Device *device)
{
    // 将运行标志置1
    device->is_running = 1;
    int result = pthread_create(&device->background_thread, NULL, device->vptr->device_backgound_func, device);
    if (result < 0)
    {
        device->is_running = 0;
        return -1;
    }
    return 0;
}

void app_device_stop(Device *device)
{
    if (device->is_running > 0)
    {
        device->is_running = 0;
        pthread_cancel(device->background_thread);
        pthread_join(device->background_thread, NULL);
    }
}

int app_device_write(Device *device, void *ptr, int len)
{
    log_trace("Device: write");
    unsigned char *buf = ptr;
    if (len != buf[0] + buf[1] + 2)
    {
        log_warn("Device write fail, message len wrong");
        return -1;
    }

    // 将数据写入发送缓存
    int result = app_buffer_write(device->send_buffer, ptr, len);
    log_trace("Device: write end");
    if (result < 0)
    {
        return -1;
    }

    // 注册send_task
    app_task_registerTask(SEND_TYPE, device->vptr->send_task, device);
    return 0;
}

void app_device_registerReadCallback(Device *device, void (*callback)(Device *, void *, int))
{
    device->vptr->recv_callback = callback;
}
