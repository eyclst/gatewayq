/**
 * 通过libcurl实现的简单HTTP客户端
 * 可以从服务器查询版本号
 * 可以下载最新的固件文件
 */
#if !defined(__OTA_HTTP_H__)
#define __OTA_HTTP_H__


typedef struct VersionStruct
{
    int major;
    int minor;
    int fix;
} Version;

#define VERSION_URL "http://example.com/version"
#define FIRMWARE_URL "https://raw.githubusercontent.com/skiinder/car-data/master/pom.xml"
#define FIRMWARE_SHA_URL "https://example.com/firmware.md5"

/**
 * @brief 请求VERSION_URL 获取版本号
 *        版本号会以JSON形式返回，形式如下：
 *        {"major":2,"minor":1,"fix":1}
 *
 * @param ver 版本号指针
 * @return int 成功返回0, 失败返回-1
 */
int ota_http_getVersion(Version *ver);

/**
 * @brief 请求FIRMWARE_URL，下载最新固件
 *
 * @param filename 固件文件名
 * @return int 0 成功 -1失败
 */
int ota_http_downloadFirmware(const char *filename);



#endif // __OTA_HTTP_H__
