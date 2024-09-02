#include "app_router.h"
#include "app_mqtt.h"
#include "app_message.h"
#include "log.c/log.h"

static Device *device_list[MAX_DEVICE_COUNT];
static int device_count = 0;

/**
 * @brief 当MQTT客户端收到消息之后，会调用这个回调函数
 *
 * @param ptr
 * @param len
 * @return int
 */
static int app_router_mqttCallback(void *ptr, int len)
{
    log_trace("MQTT Callback started");
    unsigned char buf[128];
    Message message;
    app_message_initByJson(&message, ptr);
    log_trace("Message parsed");
    for (int i = 0; i < device_count; i++)
    {
        if (message.connection_type == device_list[i]->connection_type)
        {
            int result = app_message_saveBinary(&message, buf, 128);
            log_trace("Message binary saved");
            // app_message_destroy(&message);
            log_trace("Message destroyed");
            if (result == -1)
            {
                log_warn("Router message buf not enough.");
                return -1;
            }
            app_device_write(device_list[i], buf, result);
            log_trace("Message writed to device");
            return 0;
        }
    }
    log_warn("No suitable device for message %s", ptr);
    app_message_destroy(&message);
    return -1;
}

/**
 * @brief 下游设备收到消息之后，会调用这个回调函数
 *
 * @param ptr 消息指针
 * @param len 长度
 */
static void app_router_deviceCallback(Device *device, void *ptr, int len)
{
    unsigned char buf[1024];
    Message message;
    if (app_message_init(&message, device->connection_type, ptr, len) == 0)
    {
        log_trace("Message created");
    }
    int result = app_message_printJson(&message, buf, 1024);
    log_trace("Message json Printed: %s", buf);
    if (result < 0)
    {
        log_error("Router device buf not enough!");
    }
    else
    {
        app_mqtt_send(buf, result);
        log_trace("Message %s send", buf);
    }
    app_message_destroy(&message);
}

int app_router_init()
{
    if (app_mqtt_init() < 0)
    {
        return -1;
    }
    app_mqtt_registerRecvCallback(app_router_mqttCallback);
    return 0;
}

void app_router_close()
{
    app_mqtt_close();
    for (int i = 0; i < device_count; i++)
    {
        app_device_stop(device_list[i]);
        app_device_close(device_list[i]);
    }
}

int app_router_registerDevice(Device *device)
{
    if (device_count >= MAX_DEVICE_COUNT)
    {
        return -1;
    }

    device_list[device_count++] = device;
    app_device_registerReadCallback(device, app_router_deviceCallback);
    app_device_start(device);
    return 0;
}
