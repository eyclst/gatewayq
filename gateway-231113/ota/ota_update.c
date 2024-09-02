#define _POSIX_C_SOURCE 199309
#include "ota_update.h"
#include <unistd.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include "log.c/log.h"

/**
 * @brief 检查是否需要更新
 *
 * @return int 0 需要 -1 不需要
 */
static int ota_update_checkUpdate()
{
    int result;
    Version curr_ver = CURRENT_VERSION;
    Version ver;
    log_info("Current version %d.%d.%d", curr_ver.major, curr_ver.minor, curr_ver.fix);

    // 获取服务器版本号
    result = ota_http_getVersion(&ver);
    if (result < 0)
    {
        log_error("Remote version get failed");
        return -1;
    }
    log_info("Remote version %d.%d.%d", ver.major, ver.minor, ver.fix);

    // 比较服务器和本地版本号
    if (curr_ver.major < ver.major)
        return 0;

    if (curr_ver.major > ver.major)
        return -1;

    if (curr_ver.minor < ver.minor)
        return 0;

    if (curr_ver.minor > ver.minor)
        return -1;

    if (curr_ver.fix < ver.fix)
        return 0;
    return -1;
}

void ota_update_run()
{
    int result;
    while (1)
    {
        // 检查版本号
        log_info("Checking server version...");
        result = ota_update_checkUpdate();
        if (result < 0)
        {
            sleep(86400);
            continue;
        }

        // 开始OTA
        log_info("Updating...");

        // 下载firmware
        result = ota_http_downloadFirmware(TEMP_FIRMWARE);
        if (result < 0)
        {
            log_info("Retrying in one hour");
            sleep(3600);
            continue;
        }

        log_info("Update complete, rebooting...");

        // 更新完毕，重新启动开发版
        reboot(RB_AUTOBOOT);
    }
}