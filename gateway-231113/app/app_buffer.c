#include "app_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "log.c/log.h"

static pthread_mutex_t lock_initializer = PTHREAD_MUTEX_INITIALIZER;

int app_buffer_init(Buffer *buffer, int total_len)
{
    log_trace("Buffer %p initializing...", buffer);
    bzero(buffer, sizeof(Buffer));
    if (!buffer)
    {
        log_error("Buffer not valid");
        return -1;
    }

    buffer->ptr = malloc(total_len);
    if (!buffer->ptr)
    {
        log_error("Not enough memory for buffer %p", buffer);
        // 申请失败
        return -1;
    }
    // 初始化buffer同步锁
    memcpy(&buffer->lock, &lock_initializer, sizeof(pthread_mutex_t));

    buffer->total_len = total_len;
    log_trace("Buffer %p initialized", buffer);
    return 0;
}

void app_buffer_close(Buffer *buffer)
{
    log_trace("Closing buffer %p", buffer);
    if (!buffer)
    {
        log_error("Buffer not valid");
        return;
    }
    if (buffer->ptr)
    {
        free(buffer->ptr);
        buffer->ptr = NULL;
    }
    buffer->total_len = 0;
    buffer->offset = 0;
    buffer->len = 0;
    log_trace("Buffer %p closed", buffer);
}

int app_buffer_read(Buffer *buffer, void *ptr, int len)
{
    log_trace("Read from buffer %p to ptr %p of len %d", buffer, ptr, len);
    int result = 0;
    // buffer判空
    if (!buffer)
    {
        log_error("Buffer not valid");
        return -1;
    }

    if (!ptr)
    {
        log_error("Read ptr is NULL");
        return -1;
    }

    // 开始读
    pthread_mutex_lock(&buffer->lock);
    result = buffer->len > len ? len : buffer->len;

    if (result == 0)
    {
        pthread_mutex_unlock(&buffer->lock);
        log_debug("Nothing to read from buffer %p", buffer);
        return 0;
    }

    // 先判断offset + result 是否大于total_len
    if (buffer->offset + result > buffer->total_len)
    {
        // 分两段读取
        int len1 = buffer->total_len - buffer->offset;
        int len2 = result - len1;
        unsigned char *temp = ptr;
        memcpy(ptr, buffer->ptr + buffer->offset, len1);
        memcpy(temp + len1, buffer->ptr, len2);
        // 处理buffer offset
        buffer->offset = len2;
    }
    else
    {
        // 直接读取
        memcpy(ptr, buffer->ptr + buffer->offset, result);
        buffer->offset += result;
    }

    // buffer len
    buffer->len -= result;

    pthread_mutex_unlock(&buffer->lock);
    log_debug("Buffer status: offset %d, len %d", buffer->offset, buffer->len);

    return result;
}

int app_buffer_write(Buffer *buffer, void *ptr, int len)
{
    log_trace("Write to buffer %p from ptr %p of len %d", buffer, ptr, len);
    // buffer判空
    if (!buffer)
    {
        log_error("Buffer not valid");
        return -1;
    }

    if (!ptr)
    {
        log_error("Read ptr is NULL");
        return -1;
    }

    // 加锁
    pthread_mutex_lock(&buffer->lock);

    // 判断buffer剩余空间
    if (buffer->total_len - buffer->len < len)
    {
        // 说明buffer空间不够
        pthread_mutex_unlock(&buffer->lock);
        log_error("Buffer %p overflowed", buffer);
        return -1;
    }

    // 判断一段写还是两段写
    // 首先需要找到写offset
    int write_offset = buffer->offset + buffer->len;
    if (write_offset > buffer->total_len)
    {
        write_offset -= buffer->total_len;
    }

    // 判断从写offset到buffer末尾的长度有多长，是否能够装下要写的内容
    if (buffer->total_len - write_offset < len)
    {
        // 分两段写
        int len1 = buffer->total_len - write_offset;
        int len2 = len - len1;
        unsigned char *temp = ptr;
        memcpy(buffer->ptr + write_offset, ptr, len1);
        memcpy(buffer->ptr, temp + len1, len2);
    }
    else
    {
        // 一段写
        memcpy(buffer->ptr + write_offset, ptr, len);
    }
    buffer->len += len;
    pthread_mutex_unlock(&buffer->lock);
    return 0;
}
