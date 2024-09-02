#include "app_bluetooth.h"
#include <string.h>
#include <unistd.h>
#include "log.c/log.h"

static unsigned char recv_buf[128];
static int recv_offset = 0;
static unsigned char fix_header[] = {0xF1, 0xDD};

/**
 * @brief 移除接收缓存的前len字节
 *
 * @param len
 */
static void app_bluetooth_removeBuffer(int len)
{
    if (len > 0)
    {
        recv_offset -= len;
        memcpy(recv_buf, recv_buf + len, recv_offset);
    }
}
/**
 * @brief 找到下一个关心的头部（f1 dd 或者 4f 4b)
 *
 * @return int -1表示没找到, 0表示ACK，1表示收到数据
 */
static int app_bluetooth_seekHeader()
{
    int index = 0, result = -1;
    for (index = 0; index < recv_offset - 3; index++)
    {
        if (memcmp(recv_buf + index, fix_header, 2) == 0)
        {
            // 找到处理
            result = 1;
            break;
        }
        if (memcmp(recv_buf + index, "OK", 2) == 0)
        {
            result = 0;
            break;
        }
    }
    // 没找到处理
    app_bluetooth_removeBuffer(index);
    return result;
}

static int app_bluetooth_waitACK(Device *device)
{
    unsigned char buf[5];
    int len = read(device->fd, buf, 5);
    if (len != 4)
    {
        return -1;
    }
    if (memcmp(buf, "OK\r\n", 4) != 0)
    {
        return -1;
    }
    return 0;
}

int app_bluetooth_checkStatus(Device *device)
{
    write(device->fd, "AT\r\n", 4);
    return app_bluetooth_waitACK(device);
}

int app_bluetooth_setBaudRate(Device *device, BTBaudRate baud_rate)
{
    unsigned char buf[10];
    memcpy(buf, "AT+BAUD8\r\n", 10);
    memcpy(buf + 7, &baud_rate, 1);
    write(device->fd, buf, 10);
    return app_bluetooth_waitACK(device);
}

int app_bluetooth_setNetID(Device *device, char *net_id)
{
    unsigned char buf[14];
    memcpy(buf, "AT+NETID1111\r\n", 14);
    memcpy(buf + 8, net_id, 4);
    write(device->fd, buf, 14);
    return app_bluetooth_waitACK(device);
}

int app_bluetooth_setMAddr(Device *device, char *maddr)
{
    unsigned char buf[14];
    memcpy(buf, "AT+MADDR1111\r\n", 14);
    memcpy(buf + 8, maddr, 4);
    write(device->fd, buf, 14);
    return app_bluetooth_waitACK(device);
}

int app_bluetooth_reset(Device *device)
{
    write(device->fd, "AT+RESET\r\n", 10);
    return app_bluetooth_waitACK(device);
}

// 我们需要发送的数据为AT+MESH [00 id两字节 msg n字节 0d 0a]
void app_bluetooth_preSend(Device *device, void *ptr, int *p_len)
{
    // 首先将消息拷贝出来
    unsigned char *buf = ptr;
    if (buf[0] != 2)
    {
        // ID长度不对
        log_warn("Id not valid");
        *p_len = 0;
        return;
    }

    if (buf[1] > 12)
    {
        // 消息过长
        log_warn("Msg too long");
        *p_len = 0;
        return;
    }

    unsigned char temp[14];
    int len = buf[0] + buf[1];
    if (len + 2 != *p_len)
    {
        // 这种情况不对
        log_warn("Total len not valid");
        *p_len = 0;
        return;
    }

    memcpy(temp, buf + 2, len);

    // 构成消息
    memcpy(buf, "AT+MESH", 8);
    memcpy(buf + 8, temp, len);
    memcpy(buf + 8 + len, "\r\n", 2);

    *p_len = 8 + len + 2;
    log_trace("Message %s, len %d", buf, *p_len);

    // 由于蓝牙是慢速设备
    device->last_write_ts = app_common_getCurrentTimestamp();
}

void app_bluetooth_preRecv(Device *device, void *ptr, int *p_len)
{
    // 将接收来的数据写进缓冲区
    memcpy(recv_buf + recv_offset, ptr, *p_len);
    recv_offset += *p_len;
    // 默认情况下消息不需要处理
    *p_len = 0;

    // 判断缓冲区数据长度
    if (recv_offset < 4)
    {
        // 消息不完整
        return;
    }

    // 找下一条消息头
    int result = app_bluetooth_seekHeader();
    switch (result)
    {
    case 0:
        if (recv_offset < 4)
        {
            // ACK信息不全，留待下一次处理
            return;
        }
        // 允许设备的下次写
        device->last_write_ts = 0;
        // 将ack信息处理掉
        app_bluetooth_removeBuffer(4);
        break;
    case 1:
        // 处理收到的数据
        unsigned char *temp = ptr;
        if (recv_buf[2] > recv_offset - 3)
        {
            // 缓存区的消息不完整
            return;
        }
        // 拷贝消息 最终消息格式应该是 id长度 msg长度 id msg
        temp[0] = 2;
        temp[1] = recv_buf[2] - 4;
        // 拷贝id
        memcpy(temp + 2, recv_buf + 3, 2);
        // 拷贝msg
        memcpy(temp + 4, recv_buf + 7, temp[1]);
        // 消息总长度
        *p_len = temp[0] + temp[1] + 2;
        // 处理掉消息
        app_bluetooth_removeBuffer(recv_buf[2] + 3);
        break;
    default:
        break;
    }
}
