#include "log.c/log.h"
#include "app/app_buffer.h"
#include "app/app_task.h"
#include "app/app_mqtt.h"
#include "app/app_message.h"
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief Buffer测试用例
 *
 * @return int 0表示成功 -1表示失败
 */
int test_buffer()
{
    char ptr[16];
    Buffer buffer;

    app_buffer_init(&buffer, 16);
    assert(buffer.total_len == 16);

    app_buffer_write(&buffer, "I love you", 11);
    assert(buffer.offset == 0);
    assert(buffer.len == 11);

    app_buffer_read(&buffer, ptr, 5);
    assert(buffer.offset == 5);
    assert(buffer.len == 6);
    assert(ptr[0] == 'I');

    app_buffer_write(&buffer, "ABCDEF", 7);
    assert(buffer.offset == 5);
    assert(buffer.len == 13);

    app_buffer_read(&buffer, ptr, 10);
    assert(buffer.offset == 15);
    assert(buffer.len == 3);
    assert(ptr[0] == 'e');

    app_buffer_read(&buffer, ptr, 3);
    assert(buffer.offset == 2);
    assert(buffer.len == 0);
    assert(ptr[0] == 'E');

    app_buffer_close(&buffer);
    assert(buffer.ptr == NULL);
    assert(buffer.total_len == 0);

    return 0;
}

void task(void *argv)
{
    log_info("Sub Task executing...");
    int *p = argv;
    *p = 10;
}

void test_task(const char *filename)
{
    int temp = 5;
    app_task_init(filename);
    app_task_registerTask(SEND_TYPE, task, &temp);
    log_info("Task created");

    sleep(3);
    assert(temp == 10);

    app_task_close();
    log_info("Task manager test succeed!");
}

int MQTT_print(void *ptr, int len)
{
    char *str = ptr;
    assert(strlen(str) == len - 1);
    log_info("Received msg %s", str);
}

void test_MQTT()
{
    int result = -1;
    result = app_mqtt_init();
    assert(result == 0);
    app_mqtt_registerRecvCallback(MQTT_print);

    result = app_mqtt_send("Today's weather is good.", 25);
    assert(result == 0);
    sleep(10);

    app_mqtt_close();
}

void test_message()
{
    char buf[1024];
    Message msg;
    app_message_initByJson(&msg, "{\"connection_type\": 10,\"id\": \"7FAA\",\"message\": \"6ABBCCDD\"}");
    app_message_printJson(&msg, buf, 1024);
    log_info("MSG: %s", buf);
}

int main(int argc, char const *argv[])
{
    log_set_level(LOG_TRACE);
    // int result = test_buffer();
    // if (result == 0)
    // {
    //     log_info("Buffer test succeed!");
    // }

    // test_task(argv[0]);

    // test_MQTT();

    test_message();

    return 0;
}
