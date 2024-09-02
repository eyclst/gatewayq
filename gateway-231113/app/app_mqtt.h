#if !defined(__APP_MQTT_H__)
#define __APP_MQTT_H__

#define ADDRESS "tcp://192.168.31.234:1883"
#define CLIENTID "17973eea-9ecb-4899-8191-692a8eee0333"
#define PUSH_TOPIC "ExamplePushTopic"
#define PULL_TOPIC "ExamplePullTopic"
#define QOS 1
#define TIMEOUT 10000L

int app_mqtt_init();

void app_mqtt_close();

void app_mqtt_registerRecvCallback(int (*callback)(void *ptr, int len));

int app_mqtt_send(void *ptr, int len);

#endif // __APP_MQTT_H__
