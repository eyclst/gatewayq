#include "app_message.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON/cJSON.h"

/**
 * @brief 将ch转化为2字节的16进制字符串
 *
 * @param ch
 * @param ptr
 */
static void app_message_hexToStr(unsigned char ch, unsigned char *ptr)
{
    unsigned char vTable[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70};
    ptr[0] = vTable[ch / 16];
    ptr[1] = vTable[ch % 16];
}
/**
 * @brief 将2字节的16进制字符串转化为一个字节
 *
 * @param ptr
 * @param p_ch
 */
static void app_message_strToHex(char *ptr, unsigned char *p_ch)
{
    *p_ch = 0;
    for (int i = 0; i < 2; i++)
    {
        *p_ch *= 16;
        if (ptr[i] >= '0' && ptr[i] <= '9')
        {
            *p_ch += ptr[i] - '0';
        }
        else if (ptr[i] >= 'A' && ptr[i] <= 'F')
        {
            *p_ch += ptr[i] - 'A' + 10;
        }
    }
}

int app_message_initByJson(Message *message, char *json_string)
{
    // 解析Json
    cJSON *json = cJSON_Parse(json_string);
    if (!json)
    {
        // 要么Json串不合法，要么内存不足
        return -1;
    }

    // 读取Connection type
    cJSON *connection_type_json = cJSON_GetObjectItem(json, "connection_type");
    message->connection_type = connection_type_json->valueint;

    // 读取其他数据
    cJSON *id_hex_string = cJSON_GetObjectItem(json, "id");
    message->id_len = strlen(id_hex_string->valuestring);
    if (message->id_len % 2 != 0)
    {
        // 释放cJSON
        cJSON_Delete(json);
        return -1;
    }
    message->id_len /= 2;

    // 读取msg_len
    cJSON *msg_hex_string = cJSON_GetObjectItem(json, "message");
    message->msg_len = strlen(msg_hex_string->valuestring);
    if (message->msg_len % 2 != 0)
    {
        // 释放cJSON
        cJSON_Delete(json);
        return -1;
    }
    message->msg_len /= 2;

    message->payload = malloc(message->id_len + message->msg_len);
    if (!message->payload)
    {
        /// 释放cJSON
        cJSON_Delete(json);
        return -1;
    }

    // 读取message_id的数据
    for (int i = 0; i < message->id_len; i++)
    {
        app_message_strToHex(id_hex_string->valuestring + 2 * i, message->payload + i);
    }

    // 读取msg数据
    for (int i = 0; i < message->msg_len; i++)
    {
        app_message_strToHex(msg_hex_string->valuestring + 2 * i, message->payload + i + message->id_len);
    }

    // 释放cJSON
    cJSON_Delete(json);
    return 0;
}

int app_message_printJson(Message *message, void *ptr, int len)
{
    // 将id和msg都转化为字符串
    int i = 0;
    unsigned char *id_str = malloc(message->id_len * 2 + 1);
    if (!id_str)
    {
        return -1;
    }

    unsigned char *msg_str = malloc(message->msg_len * 2 + 1);
    if (!msg_str)
    {
        free(id_str);
        return -1;
    }

    for (i = 0; i < message->id_len; i++)
    {
        app_message_hexToStr(message->payload[i], id_str + 2 * i);
    }
    id_str[message->id_len * 2] = 0;

    for (i = 0; i < message->msg_len; i++)
    {
        app_message_hexToStr(message->payload[message->id_len + i], msg_str + 2 * i);
    }
    msg_str[message->msg_len * 2] = 0;

    cJSON *json_object = cJSON_CreateObject();
    // 向json中添加整数
    cJSON_AddNumberToObject(json_object, "connection_type", message->connection_type);
    cJSON_AddStringToObject(json_object, "id", (char*)id_str);
    cJSON_AddStringToObject(json_object, "message", (char*)msg_str);

    char *json_string = cJSON_PrintUnformatted(json_object);
    free(id_str);
    free(msg_str);
    cJSON_Delete(json_object);
    if (!json_string)
    {
        return -1;
    }

    int temp_len = strlen(json_string) + 1;

    if (len < temp_len)
    {
        cJSON_free(json_string);
        return -1;
    }

    memcpy(ptr, json_string, strlen(json_string) + 1);
    cJSON_free(json_string);
    return strlen(json_string) + 1;
}

int app_message_init(Message *message, ConnectionType type, void *ptr, int len)
{
    bzero(message, sizeof(Message));
    message->connection_type = type;

    // 读取id长度
    memcpy(&message->id_len, ptr, 1);
    // 读取msg长度
    memcpy(&message->msg_len, ptr + 1, 1);

    if (message->id_len + message->msg_len + 2 != len)
    {
        return -1;
    }

    // 读取id 和 msg
    message->payload = malloc(message->id_len + message->msg_len);
    if (!message->payload)
    {
        return -1;
    }
    memcpy(message->payload, ptr + 2, message->id_len + message->msg_len);
    return 0;
}

int app_message_saveBinary(Message *message, void *ptr, int len)
{
    if (message->id_len + message->msg_len + 2 > len)
    {
        return -1;
    }

    // 写id长度
    memcpy(ptr, &message->id_len, 1);
    // 写msg长度
    memcpy(ptr + 1, &message->msg_len, 1);
    // 写payload
    memcpy(ptr + 2, message->payload, message->id_len + message->msg_len);
    return message->id_len + message->msg_len + 2;
}

void app_message_destroy(Message *message)
{
    if (message->payload)
    {
        free(message->payload);
        message->payload = NULL;
    }
}
