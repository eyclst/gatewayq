#if !defined(__APP_ROUTER_H__)
#define __APP_ROUTER_H__

#include "app_device.h"

#define MAX_DEVICE_COUNT 10

int app_router_init();

void app_router_close();

int app_router_registerDevice(Device *device);

#endif // __APP_ROUTER_H__
