/**
  ******************************************************************************
  * @file       sla_cmpower_w1.c
  * @brief      Source file for slaven cmpower w1
  * @details 
  * @author     Slaven
  * @data       2024-06-08
  * @version    V1.0  
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "sla_nvs_drv.h"
#include "sla_led_drv.h"
#include "sla_relay_drv.h"
#include "sla_btn_drv.h"
#include "smartconfig.h"
#include "mqtt_client.h"
#include "cJSON.h"

static const char *TAG = "app_main";

static esp_mqtt_client_handle_t mqtt_client;
static char mqtt_user_topic[USER_TOPIC_MAX_LEN+1];
static char mqtt_topic_set[1+USER_TOPIC_MAX_LEN+1+3];
static char mqtt_topic_get[1+USER_TOPIC_MAX_LEN+1+3];
static char mqtt_topic_status[1+USER_TOPIC_MAX_LEN+1+6];
static char *socket_main_on_payload  = "{\"socket\":\"main\",\"onoff\":\"on\"}";
static char *socket_main_off_payload = "{\"socket\":\"main\",\"onoff\":\"off\"}";
static char *socket_sub_on_payload   = "{\"socket\":\"sub\",\"onoff\":\"on\"}";
static char *socket_sub_off_payload  = "{\"socket\":\"sub\",\"onoff\":\"off\"}";

static int is_cloud_connected;
static int socket_main_sta;
static int socket_sub_sta;
static int led_timer_is_init;
static int led_timer_is_running;
static void *led_timer_handle;

static int sla_is_cloud_connected(void)
{
    return is_cloud_connected;
}

static void _sla_led_timer_handler(void *p_timer)
{
    static int onoff;

    onoff ^= 1;
    sla_led_r_ctrl(0);
    sla_led_b_ctrl(onoff);
}

static esp_err_t _sla_sys_evt_cb(sys_type_t sys_type, void *args)
{
    static int first, flag;
    
    ESP_LOGW(TAG, "system type: %d", sys_type);

    if (0 == first) {
        if (sla_is_wifi_config()) {
            flag = 1;
        } else {
            flag = 0;
        }

        first = 1;
    }

    if (sla_is_wifi_config() && flag) {
        if (sla_is_wifi_connected()) {
            sla_led_r_ctrl(0);
            sla_led_b_ctrl(0);
        } else {
            sla_led_r_ctrl(1);
            sla_led_b_ctrl(0);
        }        
    } else {
        if (0 == led_timer_is_init) {
            led_timer_is_init = 1;
            led_timer_handle = xTimerCreate("led timer", 500, 1, 0, _sla_led_timer_handler);
            led_timer_is_running = 1;
            xTimerStart(led_timer_handle, 0);            
        } else {
//            if (0 == led_timer_is_running) {
//                xTimerStart(led_timer_handle, 0);
//            }

            if (sla_is_wifi_connected()) {
                xTimerStop(led_timer_handle, 0);
                sla_led_r_ctrl(0);
                sla_led_b_ctrl(1);
            }

            if (sla_is_cloud_connected()) {
                flag = 1;
                sla_led_r_ctrl(0);
                sla_led_b_ctrl(0);                
            }
        }
    }

    return ESP_OK;
}

static void _sla_prepare_topic(char *user_topic)
{
    size_t input_len = strlen(user_topic);
    if (input_len > USER_TOPIC_MAX_LEN) {
        ESP_LOGE(TAG, "user topic exceeds the maximum length of %d bytes", USER_TOPIC_MAX_LEN);
    }

    sprintf(mqtt_topic_set,    "/%s/set",    user_topic);
    sprintf(mqtt_topic_get,    "/%s/get",    user_topic);
    sprintf(mqtt_topic_status, "/%s/status", user_topic);

    ESP_LOGW(TAG, "prepare topic:%s,%s,%s", mqtt_topic_set, mqtt_topic_get, mqtt_topic_status);
}

static char* get_payload(const char *socket_type, int status)
{
    if (!strcmp(socket_type, "main")) {
        if (status) {
            return socket_main_on_payload;
        } else {
            return socket_main_off_payload;
        }
    }

    if (!strcmp(socket_type, "sub")) {
        if (status) {
            return socket_sub_on_payload;
        } else {
            return socket_sub_off_payload;
        }
    }
    
    return "error";    
}

static void sla_factory_reset(void);
static void _sla_button_cb(drv_btn_type_t btn_type, uint32_t pin_num)
{
    ESP_LOGI(TAG, "button[%d] type: %d", pin_num, btn_type);

    if (DRV_BUTTON_PRESS_SHORT == btn_type) {
        socket_main_sta ^= 1;
        sla_relay_all_ctrl(socket_main_sta);
        socket_sub_sta = socket_main_sta;
        sla_led_w_ctrl(socket_sub_sta);        
        if (sla_is_cloud_connected()) {
            esp_mqtt_client_publish(mqtt_client, mqtt_topic_status, get_payload("main", socket_main_sta), 0, 1, 0);
            vTaskDelay(10);
            esp_mqtt_client_publish(mqtt_client, mqtt_topic_status, get_payload("sub", socket_sub_sta), 0, 1, 0);
        }
    } else if (DRV_BUTTON_PRESS_HOLD == btn_type) {
        sla_factory_reset();
    } else if (DRV_BUTTON_PRESS_RELEASE == btn_type) {
        esp_restart();
    }
}

static void parse_socket_and_onoff(const char *json_string, char *socket_type, char *status)
{
    cJSON *json = cJSON_Parse(json_string);
    if (json == NULL) {
        ESP_LOGE(TAG, "error parsing JSON");
        return;
    }

    const cJSON *socket = cJSON_GetObjectItemCaseSensitive(json, "socket");
    const cJSON *onoff = cJSON_GetObjectItemCaseSensitive(json, "onoff");

    if (cJSON_IsString(socket) && (socket->valuestring != NULL)) {
//        ESP_LOGW(TAG, "socket: %s", socket->valuestring);
        strcpy(socket_type, socket->valuestring);
    } else {
        ESP_LOGE(TAG, "socket not found or not a string");
    }

    if (cJSON_IsString(onoff) && (onoff->valuestring != NULL)) {
//        ESP_LOGW(TAG, "onoff: %s", onoff->valuestring);
        strcpy(status, onoff->valuestring);
    } else {
        ESP_LOGE(TAG, "onoff not found or not a string");
    }

    cJSON_Delete(json);
}

static void mqtt_data_cb(esp_mqtt_client_handle_t client, char *topic, int topic_len, char *data, int data_len)
{
//    printf("TOPIC=%.*s\r\n", topic_len, topic);
//    printf("DATA=%.*s\r\n", data_len, data);

    char socket_type[10] = {0}, status[10] = {0};
    int onoff = 0;
    
    parse_socket_and_onoff(data, socket_type, status);
    ESP_LOGW(TAG, "socket:%s, onoff:%s", socket_type, status);

    if (0 != status[0] && !strcmp(status, "on")) {
        onoff = 1;
    } else if (0 != status[0] && !strcmp(status, "off")) {
        onoff = 0;
    }

    if (!strncmp(topic, mqtt_topic_set, topic_len)) {
        if (0 != socket_type[0] && !strcmp(socket_type, "main")) {
            //main relay ctrl            

            socket_main_sta = onoff;        
            esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("main", socket_main_sta), 0, 1, 0);        
            sla_relay_main_ctrl(socket_main_sta);
            
            if (0 == socket_main_sta && 1 == socket_sub_sta) {
                vTaskDelay(10);
                esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("sub", 0), 0, 1, 0);
                socket_sub_sta = 0;
                sla_relay_sub_ctrl(socket_sub_sta);
                sla_led_w_ctrl(socket_sub_sta);        
            }
        }

        if (0 != socket_type[0] && !strcmp(socket_type, "sub")) {
            //sub relay ctrl            

            socket_sub_sta = onoff;

            if (0 == socket_main_sta) {
                esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("sub", 0), 0, 1, 0);
                socket_sub_sta = 0;
            } else {
                esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("sub", socket_sub_sta), 0, 1, 0);
            }
            sla_relay_sub_ctrl(socket_sub_sta);
            sla_led_w_ctrl(socket_sub_sta);        
        }
    } else if (!strncmp(topic, mqtt_topic_get, topic_len)) {
        if (0 != socket_type[0] && !strcmp(socket_type, "main")) {
            esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("main", socket_main_sta), 0, 1, 0);
        }

        if (0 != socket_type[0] && !strcmp(socket_type, "sub")) {
            esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("sub", socket_sub_sta), 0, 1, 0);
        }
    }
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            is_cloud_connected = 1;
            // add system cb
            _sla_sys_evt_cb(SYS_CLOUD_CONNECTED, NULL);

            esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("main", socket_main_sta), 0, 1, 0);
            vTaskDelay(10);
            esp_mqtt_client_publish(client, mqtt_topic_status, get_payload("sub", socket_sub_sta), 0, 1, 0);
                
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic_set, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic_get, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            is_cloud_connected = 0;
            // add system cb
            _sla_sys_evt_cb(SYS_CLOUD_DISCONNECTED, NULL);            
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            mqtt_data_cb(client, event->topic, event->topic_len, event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    char mqtt_broker_url[65] = {0};

    sla_get_mqtt_broker_url_and_user_topic(mqtt_broker_url, mqtt_user_topic);
    _sla_prepare_topic(mqtt_user_topic);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = mqtt_broker_url,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    sla_nvs_init();
    
    sla_led_init();
    
    sla_relay_init();

    sla_button_init(10000, _sla_button_cb);

    sla_sys_evt_cb_reg(_sla_sys_evt_cb);
    
    sla_smartconfig_init();

    mqtt_app_start();
}

static void sla_factory_reset(void)
{
    sla_wifi_reset();
    sla_relay_all_ctrl(0);
    sla_led_w_ctrl(0);
    sla_led_b_ctrl(0);
    sla_led_r_ctrl(1);
}
