#if !defined(__APP_DEVICE_H__)
#define __APP_DEVICE_H__
#include <pthread.h>
#include "app_buffer.h"
#include "app_common.h"

typedef enum ConnectionType
{
    BLUETOOTH = 1,
    LORA,
    OTHER
} ConnectionType;

typedef enum DeviceType
{
    SERIAL = 1,
    IIC,
    SPI,
    CAN,
    GENERAL
} DeviceType;

struct DeviceVTalbe;

typedef struct DeviceStruct
{
    struct DeviceVTable *vptr;
    const char *device_name;
    int fd;
    DeviceType type;
    ConnectionType connection_type;
    long last_write_ts;  // 上次写入的时间戳, 如果设备写入需要间隔，通过这个变量控制
    Buffer *send_buffer; // 写缓存
    Buffer *recv_buffer; // 读缓存

    int is_running;              // 设备后台线程运行标志
    pthread_t background_thread; // 后台线程ID
} Device;

struct DeviceVTable
{
    void (*recv_callback)(Device *, void *, int);    // 收到数据以后的回调函数
    void *(*device_backgound_func)(void *);          // 设备后台同步线程逻辑
    void (*pre_recv)(Device *, void *ptr, int *len); // 消息收到以后的预处理
    void (*pre_send)(Device *, void *ptr, int *len); // 消息最终写入设备前的预处理
    void (*recv_task)(void *);                       // 处理缓存数据的逻辑
    void (*send_task)(void *);                       // 收到写数据后的发送逻辑
};

/**
 * @brief 设备初始化
 *
 * @param device
 * @param device_name 设备名称
 * @return int
 */
int app_device_init(Device *device, const char *device_name);

/**
 * @brief 关闭设备
 *
 * @param device
 */
void app_device_close(Device *device);

/**
 * @brief 启动设备后台线程
 *
 * @param device
 * @return int
 */
int app_device_start(Device *device);

/**
 * @brief 停止设备后台线程
 *
 * @param device
 */
void app_device_stop(Device *device);

/**
 * @brief 向设备中写数据
 *
 * @param device
 * @param ptr
 * @param len
 * @return int
 */
int app_device_write(Device *device, void *ptr, int len);

/**
 * @brief 注册设备读数据回调函数
 *
 * @param device
 * @param callback
 */
void app_device_registerReadCallback(Device *device, void (*callback)(Device *, void *, int));

#endif // __APP_DEVICE_H__
