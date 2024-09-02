#if !defined(__APP_SERIAL_H__)
#define __APP_SERIAL_H__

#include <termios.h>
#include <unistd.h>
#include "app_device.h"

// 波特率
typedef enum BaudRate
{
    BAUD_2400 = B2400,
    BAUD_4800 = B4800,
    BAUD_9600 = B9600,
    BAUD_19200 = B19200,
    BAUD_38400 = B38400,
    BAUD_57600 = B57600,
    BAUD_115200 = B115200,
} BaudRate;

// 停止位
typedef enum StopBits
{
    ONE = 0,
    TWO = CSTOPB
} StopBits;

// 数据位
typedef enum DataBits
{
    DB7 = CS7,
    DB8 = CS8
} DataBits;

// 校验位
typedef enum Parity
{
    NONE = 0,
    EVEN = PARENB,
    ODD = PARENB | PARODD
} Parity;

typedef struct SerialDeviceStruct
{
    Device super;
    BaudRate baud_rate;
    Parity parity;
    DataBits data_bits;
    StopBits stop_bits;
} SerialDevice;

int app_serial_init(SerialDevice *device, const char *filename, BaudRate baud_rate);

int app_serial_setConnectionType(SerialDevice *device, ConnectionType type);

#endif // __APP_SERIAL_H__
