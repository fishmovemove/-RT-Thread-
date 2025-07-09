/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-13     fish move move       the first version
 */
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "paho_mqtt.h"
#include <wlan_mgnt.h>

#include "TDS.h"
#include "PH.h"
#include "TUB.h"
#include "DS18B20.h"
#include <drv_gpio.h>
#include <cJSON.h>

#define DBG_TAG "mqtt"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* Wi‑Fi配置 */
#define WLAN_SSID      "Redmi"
#define WLAN_PASSWORD  "12345678"

/* MQTT配置 */
#define MQTT_HOST      "sh-3-mqtt.iot-api.com"
#define MQTT_PORT      1883
#define MQTT_CLIENTID  "spark-rtt"
#define MQTT_USERNAME  "bip24arucukcp9q2"
#define MQTT_PASSWORD  "nbYv1jeyX3"
#define MQTT_PUB_TOPIC "attributes/push"
#define MQTT_PUB_TOPIC "attributes"

#define FEED GET_PIN(D,10)

/* MQTT 资源 */
static MQTTClient client;
static char mqtt_uri[128];
static char mqtt_topic[128];
static struct rt_timer mqtt_pub_timer;
static struct rt_semaphore mqtt_pub_sem;
static rt_thread_t mqtt_pub_thread = RT_NULL;

/* MQTT 发布线程 */
static void mqtt_pub_thread_entry(void *parameter)
{
    TDS_Init();
    DS18B20_Init();
    ph_sensor_init();
    turb_adc_init();
    while (1)
    {
        if (rt_sem_take(&mqtt_pub_sem, RT_WAITING_FOREVER) == RT_EOK)
        {
            if (client.isconnected)
            {
                char payload[128];

                int temp_raw = DS18B20_Get_Temp();         // 例如 273 表示 27.3℃
                float ph = ph_sensor_get_value();          // 例如 7.28
                float tds = TDS_GetData_PPM();             // 例如 421.55
                float turb = turb_get_data();              // 例如 135.88

                rt_snprintf(payload, sizeof(payload),
                            "{\"temperature\":%d.%d,\"ph\":%d.%02d,\"tds\":%d.%02d,\"turbidity\":%d.%02d}",
                            temp_raw / 10, temp_raw % 10,
                            (int)ph, (int)(ph * 100) % 100,
                            (int)tds, (int)(tds * 100) % 100,
                            (int)turb, (int)(turb * 100) % 100);


                MQTTMessage msg = {
                    .qos = QOS0,
                    .retained = 0,
                    .payload = (void *)payload,
                    .payloadlen = strlen(payload),
                };
                MQTTPublish(&client, mqtt_topic, &msg);
                LOG_I("Published: %s", payload);
            }
        }
    }
}

#define LIGHT GET_PIN(D,9)
#define FILTER GET_PIN(D,8)
#define FEED GET_PIN(D,10)

static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data)
{
    char topic[128] = {0};
    memcpy(topic, msg_data->topicName->lenstring.data, msg_data->topicName->lenstring.len);
    ((char *)msg_data->message->payload)[msg_data->message->payloadlen] = '\0';

    LOG_I("Received on %s: %s", topic, (char *)msg_data->message->payload);

    cJSON *root = cJSON_Parse((char *)msg_data->message->payload);
    if (root)
    {
        cJSON *relay_feed = cJSON_GetObjectItem(root, "relay1");   // 喂食器
        cJSON *relay_filter = cJSON_GetObjectItem(root, "relay2"); // 过滤器
        cJSON *relay_light = cJSON_GetObjectItem(root, "relay3");  // 灯光

        if (relay_feed)
        {
            int state = 0;
            if (cJSON_IsBool(relay_feed))
                state = relay_feed->valueint;
            else if (cJSON_IsString(relay_feed))
                state = (strcmp(relay_feed->valuestring, "true") == 0) ? 1 : 0;

            rt_pin_write(FEED, state ? PIN_HIGH : PIN_LOW);
            LOG_I("Set RELAY1 (FEED, PD11): %s", state ? "ON" : "OFF");
        }

        if (relay_filter)
        {
            int state = 0;
            if (cJSON_IsBool(relay_filter))
                state = relay_filter->valueint;
            else if (cJSON_IsString(relay_filter))
                state = (strcmp(relay_filter->valuestring, "true") == 0) ? 1 : 0;

            rt_pin_write(FILTER, state ? PIN_HIGH : PIN_LOW);
            LOG_I("Set RELAY2 (FILTER, PF7): %s", state ? "ON" : "OFF");
        }

        if (relay_light)
        {
            int state = 0;
            if (cJSON_IsBool(relay_light))
                state = relay_light->valueint;
            else if (cJSON_IsString(relay_light))
                state = (strcmp(relay_light->valuestring, "true") == 0) ? 1 : 0;

            rt_pin_write(LIGHT, state ? PIN_HIGH : PIN_LOW);
            LOG_I("Set RELAY3 (LIGHT, PD12): %s", state ? "ON" : "OFF");
        }

        cJSON_Delete(root);
    }
}

void relay_gpio_init(void)
{
    rt_pin_mode(FEED, PIN_MODE_OUTPUT);
    rt_pin_mode(FILTER, PIN_MODE_OUTPUT);
    rt_pin_mode(LIGHT, PIN_MODE_OUTPUT);

    rt_pin_write(FEED, PIN_LOW);
    rt_pin_write(FILTER, PIN_LOW);
    rt_pin_write(LIGHT, PIN_LOW);
}


/* 定时器回调 */
static void mqtt_pub_timer_cb(void *parameter)
{
    rt_sem_release(&mqtt_pub_sem);
}

/* MQTT 连接状态回调 */
static void mqtt_connect_callback(MQTTClient *c)
{
    LOG_I("Connecting...");
}

static void mqtt_online_callback(MQTTClient *c)
{
    LOG_I("MQTT connected, ready.");
    paho_mqtt_subscribe(&client, QOS1, "attributes/push", mqtt_sub_callback);
    paho_mqtt_subscribe(&client, QOS1, "command/send/+", mqtt_sub_callback);

    rt_sem_init(&mqtt_pub_sem, "pub_sem", 0, RT_IPC_FLAG_FIFO);

    mqtt_pub_thread = rt_thread_create("mqtt_pub", mqtt_pub_thread_entry, RT_NULL,
                                       1024, 20, 20);
    if (mqtt_pub_thread != RT_NULL)
        rt_thread_startup(mqtt_pub_thread);

    rt_timer_init(&mqtt_pub_timer, "pub_timer", mqtt_pub_timer_cb, RT_NULL,
                  rt_tick_from_millisecond(3000),
                  RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(&mqtt_pub_timer);
}

static void mqtt_offline_callback(MQTTClient *c)
{
    LOG_W("MQTT disconnected.");
}

/* MQTT 初始化函数 */
void mqtt_client_init(void)
{
    static int init = 0;
    if (init) return;
    init = 1;

    MQTTPacket_connectData conn = MQTTPacket_connectData_initializer;
    rt_snprintf(mqtt_uri, sizeof(mqtt_uri), "tcp://%s:%d", MQTT_HOST, MQTT_PORT);
    rt_snprintf(mqtt_topic, sizeof(mqtt_topic), "%s", MQTT_PUB_TOPIC);

    client.uri = mqtt_uri;
    client.condata = conn;
    client.condata.clientID.cstring = MQTT_CLIENTID;
    client.condata.username.cstring = MQTT_USERNAME;
    client.condata.password.cstring = MQTT_PASSWORD;
    client.condata.keepAliveInterval = 60;
    client.condata.cleansession = 1;

    client.buf_size = client.readbuf_size = 2048    ;
    client.buf = malloc(client.buf_size);
    client.readbuf = malloc(client.readbuf_size);
    if (!client.buf || !client.readbuf)
    {
        LOG_E("Memory allocation failed");
        return;
    }

    client.connect_callback = mqtt_connect_callback;
    client.online_callback = mqtt_online_callback;
    client.offline_callback = mqtt_offline_callback;

    paho_mqtt_start(&client);
}

/* Wi‑Fi 回调 */
static void wifi_ready(int event, struct rt_wlan_buff *buff, void *p)
{
    LOG_I("Wi‑Fi ready");
    rt_thread_mdelay(1000); // 等待网络稳定
    mqtt_client_init();
}

static void wifi_fail(int event, struct rt_wlan_buff *buff, void *p)
{
    //LOG_W("Wi‑Fi failed");
}



/* Wi‑Fi 初始化 */
void wifi_init(void)
{
    rt_wlan_register_event_handler(RT_WLAN_EVT_READY, wifi_ready, RT_NULL);
    rt_wlan_register_event_handler(RT_WLAN_EVT_STA_CONNECTED_FAIL, wifi_fail, RT_NULL);
    rt_wlan_set_mode(RT_WLAN_DEVICE_STA_NAME, RT_WLAN_STATION);
    rt_wlan_connect(WLAN_SSID, WLAN_PASSWORD);
}
