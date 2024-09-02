#include "app_mqtt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <MQTTClient.h>
#include <assert.h>

#include "log.c/log.h"

static MQTTClient client;
static MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
static MQTTClient_deliveryToken deliveredtoken;

static int (*recvCallback)(void *ptr, int len);

// 消息发送回调
void delivered(void *context, MQTTClient_deliveryToken dt)
{
    assert(context == NULL);
    log_trace("Message with token value %d delivery confirmed", dt);
    deliveredtoken = dt;
}

// 消息到达回调
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int result = 0;
    assert(context == NULL);
    log_trace("Message from topic %.*s arrived, content %.*s", topicLen, topicName, message->payloadlen, (char *)message->payload);
    result = recvCallback(message->payload, message->payloadlen) == 0 ? 1 : 0;
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return result;
}

// 链接丢失回调
void connlost(void *context, char *cause)
{
    assert(context == NULL);
    log_fatal("MQTT Connection lost because of %s, closing...", cause);
    exit(EXIT_FAILURE);
}

int app_mqtt_init()
{
    // 创建MQTT客户端
    if (MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS)
    {
        log_warn("MQTT Client creation fails");
        return -1;
    }

    // 设置回调函数
    if (MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered) != MQTTCLIENT_SUCCESS)
    {
        app_mqtt_close();
        log_warn("MQTT set callback fail");
        return -1;
    }

    // 连接MQTT服务器
    conn_opts.connectTimeout = TIMEOUT;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS)
    {
        app_mqtt_close();
        log_warn("MQTT connect fail");
        return -1;
    }

    // 订阅话题
    if (MQTTClient_subscribe(client, PULL_TOPIC, QOS) != MQTTCLIENT_SUCCESS)
    {
        app_mqtt_close();
        log_warn("MQTT subscribe fail");
        return -1;
    }
    return 0;
}

void app_mqtt_close()
{
    // 断开连接
    MQTTClient_disconnect(client, TIMEOUT);
    // 清理客户端
    MQTTClient_destroy(&client);
}

void app_mqtt_registerRecvCallback(int (*callback)(void *ptr, int len))
{
    recvCallback = callback;
}

int app_mqtt_send(void *ptr, int len)
{
    log_trace("Message: %s, len: %d", ptr, len);
    int result = 0;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = ptr;
    pubmsg.payloadlen = len;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    deliveredtoken = 0;

    if (MQTTClient_publishMessage(client, PUSH_TOPIC, &pubmsg, &token) != MQTTCLIENT_SUCCESS)
    {
        log_warn("Message send fail");
        result = -1;
    }
    else
    {
        result = 0;
    }

    return result;
}
