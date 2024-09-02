#if !defined(__APP_MESSAGE_H__)
#define __APP_MESSAGE_H__

#include "app_device.h"

typedef struct MessageStruct
{
    ConnectionType connection_type;
    int id_len;
    int msg_len;
    unsigned char* payload;
} Message;

/**
 * @brief 使用Json初始化message
 *
 * @param message msg结构体
 * @param json_string json串
 * @return int 成功0 失败-1
 */
int app_message_initByJson(Message *message, char *json_string);

/**
 * @brief 将Message打印成Json串
 *
 * @param message
 * @param ptr 结果Json串的指针
 * @param len 结果缓存长度
 * @return int 失败-1, 成功返回字符串长度
 */
int app_message_printJson(Message *message, void *ptr, int len);

/**
 * @brief 将16进制数据转化成msg
 *
 * @param message
 * @param type 连接类型
 * @param ptr
 * @param len
 * @return int 成功0 失败-1
 */
int app_message_init(Message *message, ConnectionType type, void *ptr, int len);

/**
 * @brief 将message保存为2进制串，格式（id长度 msg长度 id msg）
 *
 * @param message
 * @param ptr 结果指针
 * @param len 结果缓存区长度
 * @return int -1失败，成功返回msg总长度
 */
int app_message_saveBinary(Message *message, void *ptr, int len);

/**
 * @brief 销毁消息
 * 
 * @param message 
 */
void app_message_destroy(Message *message);

#endif // __APP_MESSAGE_H__
