#include "ota_http.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include "cJSON/cJSON.h"
#include "log.c/log.h"

#define BUFFER_SIZE 1024

// libcurl写回调函数，用于保存下载的固件数据到文件中
static size_t file_write_callback(char *ptr, size_t size, size_t nmemb, void *file)
{
    return fwrite(ptr, size, nmemb, file);
}

// libcurl写回调函数，可以将数据保存到字节数组
static size_t str_write_callback(char *ptr, size_t size, size_t nmemb, void *str)
{
    size_t realsize = size * nmemb;
    if (realsize > strlen(str))
    {
        realsize = strlen(str);
    }

    // 将接收到的数据拷贝到str中
    memcpy(str, ptr, realsize);
    return realsize;
}

static int ota_http_curl(const char *url, curl_write_callback callback, void *data_p)
{
    CURL *curl;
    CURLcode return_code;
    long http_return_code = 0;
    int result = 0;

    // 初始化libcurl
    curl = curl_easy_init();
    if (!curl)
    {
        log_error("curl init fail");
        return -1;
    }
    // 设置请求URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    // 设置是否跟随重定向
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // 设置写回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);

    // 设置写回调函数的数据指针，用于存储响应数据
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data_p);

    // 执行HTTP请求
    return_code = curl_easy_perform(curl);
    // 请求失败
    if (return_code != CURLE_OK)
    {
        log_error("curl_easy_perform() fail: %s", curl_easy_strerror(return_code));
        result = -1;
    }
    else
    {
        // 请求返回码不对
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_return_code);
        if (http_return_code != 200)
        {
            log_debug("HTTP return code: %d", http_return_code);
            result = -1;
        }
    }
    // 清理curl
    curl_easy_cleanup(curl);
    return result;
}

/**
 * @brief 对url发起get请求，将数据保存在data中
 *
 * @param url 需要发起请求的网址
 * @param data 保存数据的字节数足
 * @return int 0 成功 -1失败
 */
static int ota_http_getData(const char *url, char *data, int len)
{
    for (int i = 0; i < len - 1; i++)
    {
        data[i] = 32;
    }
    data[len - 1] = '\0';
    return ota_http_curl(url, str_write_callback, data);
}

/**
 * @brief 计算文件sha, 并转化为hex形式
 *
 * @param filename 要计算的文件名
 * @param sha sha hex，长度为2*SHA_DIGEST_LENGTH+1
 * @return int
 */
static int ota_http_calculateSHA(const char *filename, char *sha)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        log_error("Error opening file: %s\n", filename);
        return -1;
    }

    SHA_CTX sha_ctx;
    SHA1_Init(&sha_ctx);

    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) != 0)
    {
        SHA1_Update(&sha_ctx, buffer, bytes_read);
    }

    unsigned char sha_hash[SHA_DIGEST_LENGTH];
    SHA1_Final(sha_hash, &sha_ctx);

    fclose(file);

    // Convert binary SHA hash to hexadecimal string
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&sha[i * 2], "%02x", sha_hash[i]);
    }
    sha[SHA_DIGEST_LENGTH * 2] = '\0';

    return 0;
}

/**
 * @brief 对url发起get请求，下载文件
 *
 * @param url
 * @param filename 文件名
 * @return int 0 成功 -1失败
 */
static int ota_http_downloadFile(const char *url, const char *filename)
{
    FILE *fp = NULL;

    // 打开文件准备写入固件数据
    fp = fopen(filename, "wb");
    if (!fp)
    {
        log_error("open file %s failed", filename);
        return -1;
    }

    int result = ota_http_curl(url, file_write_callback, fp);
    fclose(fp);
    if (result < 0)
    {
        unlink(filename);
    }
    return result;
}

// 获取版本号函数
int ota_http_getVersion(Version *ver)
{

    char response[4096];
    cJSON *json = NULL;

    ota_http_getData(VERSION_URL, response, 4096);

    // 解析JSON响应
    json = cJSON_Parse(response);
    if (!json)
    {
        log_error("No valid version, recieved str is: %s", cJSON_GetErrorPtr());
        return -1;
    }

    // 提取版本号数据
    cJSON *major = cJSON_GetObjectItem(json, "major");
    cJSON *minor = cJSON_GetObjectItem(json, "minor");
    cJSON *fix = cJSON_GetObjectItem(json, "fix");
    if (!major || !minor || !fix)
    {
        log_error("No valid version");
        cJSON_Delete(json);
        return -1;
    }

    // 填充版本号结构体
    ver->major = major->valueint;
    ver->minor = minor->valueint;
    ver->fix = fix->valueint;

    cJSON_Delete(json);
    return 0;
}

// 下载固件函数
int ota_http_downloadFirmware(const char *filename)
{
    int result;
    char local_sha[SHA_DIGEST_LENGTH * 2 + 1];
    char remote_sha[SHA_DIGEST_LENGTH * 2 + 1];

    // 获取服务器SHA_DIGEST
    result = ota_http_getData(FIRMWARE_SHA_URL, remote_sha, SHA_DIGEST_LENGTH * 2 + 1);
    if (result < 0)
    {
        log_error("Firmware SHA download fail");
        return -1;
    }
    // 下载文件
    result = ota_http_downloadFile(FIRMWARE_URL, filename);
    if (result < 0)
    {
        log_error("Firmware download fail");
        unlink(filename);
        return -1;
    }

    // 计算本地SHA
    ota_http_calculateSHA(filename, local_sha);

    if (strncmp(local_sha, remote_sha, 2 * SHA_DIGEST_LENGTH) != 0)
    {
        log_error("Firmware SHA verify failed");
        unlink(filename);
        return -1;
    }

    return 0;
}
