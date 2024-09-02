#if !defined(__APP_BLUETOOTH_H__)
#define __APP_BLUETOOTH_H__

#include "app_device.h"

typedef enum BTBaudRate
{
    BT_BAUD_9600 = '4',
    BT_BAUD_115200 = '8'
} BTBaudRate;

/**
 * @brief 检查蓝牙连接状态
 *
 * @param device 设备
 * @return int 是否存在
 */
int app_bluetooth_checkStatus(Device *device);

int app_bluetooth_setBaudRate(Device *device, BTBaudRate baud_rate);

int app_bluetooth_setNetID(Device *device, char *net_id);

int app_bluetooth_setMAddr(Device *device, char *maddr);

int app_bluetooth_reset(Device *device);

/**
 * @brief 网关将消息写道蓝牙设备之前的预处理
 *
 * @param device 设备
 * @param ptr 消息指针（格式：id长度 msg长度 id msg）
 * @param p_len 消息总长度
 */
void app_bluetooth_preSend(Device *device, void *ptr, int *p_len);

/**
 * @brief 将串口收到的数据转化成标准的msg格式
 *
 * @param device 设备
 * @param ptr 消息指针（格式：id长度 msg长度 id msg）
 * @param p_len 消息总长度
 */
void app_bluetooth_preRecv(Device *device, void *ptr, int *p_len);

#endif // __APP_BLUETOOTH_H__
