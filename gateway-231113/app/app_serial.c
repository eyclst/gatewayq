#include "app_serial.h"
#include "app_bluetooth.h"
#include "log.c/log.h"

static int app_serial_setBaudRate(SerialDevice *device, BaudRate baud_rate)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    device->baud_rate = baud_rate;
    // 修改属性
    cfsetspeed(&options, baud_rate);

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

static int app_serial_setDataBits(SerialDevice *device, DataBits data_bits)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    device->data_bits = data_bits;
    // 修改属性
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= data_bits;

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

static int app_serial_setParity(SerialDevice *device, Parity parity)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    device->parity = parity;
    // 修改属性
    options.c_cflag &= ~(PARENB | PARODD);
    options.c_cflag |= parity;

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

static int app_serial_setStopBits(SerialDevice *device, StopBits stop_bits)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    device->stop_bits = stop_bits;
    // 修改属性
    options.c_cflag &= ~CSTOPB;
    options.c_cflag |= stop_bits;

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

static int app_serial_setRaw(SerialDevice *device)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    // 修改属性
    cfmakeraw(&options);

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

/**
 * @brief 切换串口读取数据的阻塞方式
 *
 * @param mode 0表示不阻塞 1表示阻塞
 * @return int
 */
static int app_serial_setBlock(SerialDevice *device, int mode)
{
    struct termios options;
    // 获取属性
    if (tcgetattr(device->super.fd, &options) < 0)
        return -1;

    // 根据参数修改阻塞模式
    if (mode)
    {
        options.c_cc[VMIN] = 1;
        options.c_cc[VTIME] = 0;
    }
    else
    {
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 5;
    }

    // 设置属性
    return tcsetattr(device->super.fd, TCSAFLUSH, &options);
}

int app_serial_init(SerialDevice *device, const char *filename, BaudRate baud_rate)
{
    app_device_init(&device->super, filename);
    app_serial_setBaudRate(device, baud_rate);
    app_serial_setDataBits(device, DB8);
    app_serial_setParity(device, NONE);
    app_serial_setStopBits(device, ONE);
    app_serial_setRaw(device);
    app_serial_setBlock(device, 1);

    // 刷新串口使配置生效
    tcflush(device->super.fd, TCIOFLUSH);
    return 0;
}

int app_serial_setConnectionType(SerialDevice *device, ConnectionType type)
{
    switch (type)
    {
    case BLUETOOTH:
        // 蓝牙设备的初始化
        app_serial_setBlock(device, 0);
        app_serial_setBaudRate(device, BAUD_9600);
        // 刷新串口使配置生效
        tcflush(device->super.fd, TCIOFLUSH);
        // 检查蓝牙设备是否存在
        if (app_bluetooth_checkStatus((Device *)device) >= 0)
        {
            // 说明蓝牙的波的率为9600, 我们需要将其波特率设置为115200
            if (app_bluetooth_setBaudRate((Device *)device, BT_BAUD_115200) < 0)
            {
                return -1;
            }
            if (app_bluetooth_reset((Device *)device) < 0)
            {
                return -1;
            }
            sleep(2);
        }
        // 将串口波特率重新设置为115200
        app_serial_setBaudRate(device, BAUD_115200);
        tcflush(device->super.fd, TCIOFLUSH);

        // 继续初始化蓝牙
        if (app_bluetooth_setNetID((Device *)device, "1111") < 0)
        {
            return -1;
        }
        if (app_bluetooth_setMAddr((Device *)device, "0001") < 0)
        {
            return -1;
        }

        // 重新将串口设置为阻塞模式
        app_serial_setBlock(device, 1);
        tcflush(device->super.fd, TCIOFLUSH);

        // 设置预处理回调函数
        log_trace("Set BT callback");
        device->super.vptr->pre_recv = app_bluetooth_preRecv;
        device->super.vptr->pre_send = app_bluetooth_preSend;
        break;
    case LORA:
    default:
        break;
    }
    return 0;
}
