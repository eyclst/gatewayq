#if !defined(__APP_BUFFER_H__)
#define __APP_BUFFER_H__

#include <pthread.h>

typedef struct BufferStruct
{
    unsigned char *ptr;   // 缓存指针
    int total_len;        // 缓存的总长度
    int offset;           // 内容起始的offset
    int len;              // 内容总长度
    pthread_mutex_t lock; // 缓存锁
} Buffer;

/**
 * @brief 缓存初始化方法
 *
 * @param buffer 需要初始化的缓存指针
 * @param total_len 缓存容量
 * @return int 初始化是否成功
 */
int app_buffer_init(Buffer *buffer, int total_len);

/**
 * @brief 释放缓存
 *
 * @param buffer 缓存指针
 */
void app_buffer_close(Buffer *buffer);

/**
 * @brief 从缓存中读取数据
 *
 * @param buffer 缓存
 * @param ptr 读出的数据指针
 * @param len 读出缓冲度总长度
 * @return int 总共读了多长 0 表示没读到 -1表示异常
 */
int app_buffer_read(Buffer *buffer, void *ptr, int len);

/**
 * @brief 向缓存中写入数据
 *
 * @param buffer 缓存指针
 * @param ptr 要写入的数据指针
 * @param len 要写入的长度
 * @return int 0写入成功 -1写入失败
 */
int app_buffer_write(Buffer *buffer, void *ptr, int len);

#endif // __APP_BUFFER_H__
