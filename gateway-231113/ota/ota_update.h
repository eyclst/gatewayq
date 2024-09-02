#if !defined(__OTA_UPDATE_H__)
#define __OTA_UPDATE_H__

#include "ota_http.h"

#define CURRENT_VERSION {1, 0, 0}

#define TEMP_FIRMWARE "/tmp/atguigu-gateway.update"

/**
 * @brief ota进程主逻辑
 * 
 */
void ota_update_run();


#endif // __OTA_UPDATE_H__
