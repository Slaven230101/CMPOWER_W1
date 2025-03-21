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
#include "sla_mqtt_disc.h"
#include "sla_sy7t609_drv.h"
#include "sla_web_ota.h"

static const char *TAG = "app_main";

esp_mqtt_client_handle_t mqtt_client;
static char mqtt_object_id[OBJECT_ID_MAX_LEN+1];
static char mqtt_topic_set[OBJECT_ID_MAX_LEN+25+1];
static char mqtt_topic_get[OBJECT_ID_MAX_LEN+25+1];
static char mqtt_topic_state[OBJECT_ID_MAX_LEN+27+1];
static char mqtt_topic_availability[65];
static char mqtt_payload_not_available[17];
static const char *ha_mqtt_topic_state = "homeassistant/status";
static const char *ha_mqtt_payload_available = "online";
static const char *ha_mqtt_payload_not_available = "offline";

static char *socket_main_on_payload  = "{\"socket\":\"main\",\"onoff\":\"on\"}";
static char *socket_main_off_payload = "{\"socket\":\"main\",\"onoff\":\"off\"}";
static char *socket_sub_on_payload   = "{\"socket\":\"sub\",\"onoff\":\"on\"}";
static char *socket_sub_off_payload  = "{\"socket\":\"sub\",\"onoff\":\"off\"}";
static char *socket_ota_enable_payload       = "{\"ota\":\"Enable\"}";
static char *socket_ota_disable_payload      = "{\"ota\":\"Disable\"}";

static int is_cloud_connected;
static int socket_main_sta;
static int socket_sub_sta;
static int socket_ota_sta;
static int socket_ota_sta_last;
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

static void _sla_prepare_topic(char *object_id)
{
    size_t input_len = strlen(object_id);
    if (input_len > OBJECT_ID_MAX_LEN) {
        ESP_LOGE(TAG, "object id exceeds the maximum length of %d bytes", OBJECT_ID_MAX_LEN);
    }

    sprintf(mqtt_topic_set,          "homeassistant/switch/%s/set",          object_id);
    sprintf(mqtt_topic_get,          "homeassistant/switch/%s/get",          object_id);
    sprintf(mqtt_topic_state,        "homeassistant/switch/%s/state",        object_id);

    ESP_LOGW(TAG, "prepare topic:%s,%s,%s", mqtt_topic_set, mqtt_topic_get, mqtt_topic_state);
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
    
    if (!strcmp(socket_type, "ota")) {
        if (status) {
            return socket_ota_enable_payload;
        } else {
            return socket_ota_disable_payload;
        }
    }
    
    return "error";    
}

static void sla_factory_reset(void);
static void _sla_button_cb(drv_btn_type_t btn_type, uint32_t pin_num)
{
    ESP_LOGI(TAG, "button[%d] type: %d", pin_num, btn_type);

    if (DRV_BUTTON_PRESS_SHORT == btn_type) {
        if (socket_main_sta) {
            socket_sub_sta ^= 1;
            sla_relay_sub_ctrl(socket_sub_sta);
            sla_led_w_ctrl(socket_sub_sta);
            if (sla_is_cloud_connected()) {
                esp_mqtt_client_publish(mqtt_client, mqtt_topic_state, get_payload("sub", socket_sub_sta), 0, 1, 0);
            }            
        }
    } else if (DRV_BUTTON_PRESS_DOUBLE == btn_type) {
        socket_main_sta ^= 1;
        sla_relay_all_ctrl(socket_main_sta);
        socket_sub_sta = socket_main_sta;
        sla_led_w_ctrl(socket_sub_sta);        
        if (sla_is_cloud_connected()) {
            esp_mqtt_client_publish(mqtt_client, mqtt_topic_state, get_payload("main", socket_main_sta), 0, 1, 0);
            vTaskDelay(10);
            esp_mqtt_client_publish(mqtt_client, mqtt_topic_state, get_payload("sub", socket_sub_sta), 0, 1, 0);
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
    const cJSON *ota = cJSON_GetObjectItemCaseSensitive(json, "ota");
    
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

    if (cJSON_IsString(ota) && (ota->valuestring != NULL)) {
//        ESP_LOGW(TAG, "ota: %s", ota->valuestring);
        strcpy(socket_type, "ota");
        strcpy(status, ota->valuestring);
    } else {
        ESP_LOGE(TAG, "ota not found or not a string");
    }        

    cJSON_Delete(json);
}

static esp_err_t _check_ha_state(char *topic, int topic_len, char *data, int data_len)
{
    if (!strncmp(topic, ha_mqtt_topic_state, topic_len)) {
        if (!strncmp(data, ha_mqtt_payload_available, data_len)) {
            esp_mqtt_client_disconnect(mqtt_client);
            return ESP_OK;
        } else if (!strncmp(data, ha_mqtt_payload_not_available, data_len)) {
            return ESP_OK;
        }
    }

    return ESP_FAIL;
}

static void _sla_web_ota(void)
{    
    if (socket_ota_sta_last != socket_ota_sta) {
        socket_ota_sta_last = socket_ota_sta;
        sla_ha_mqtt_update_socket_main_config(mqtt_object_id, socket_ota_sta);
        if (socket_ota_sta) {
            sla_web_ota_start();
        } else {
            sla_web_ota_stop();
        }
    }
}

static void mqtt_data_cb(esp_mqtt_client_handle_t client, char *topic, int topic_len, char *data, int data_len)
{
//    printf("TOPIC=%.*s\r\n", topic_len, topic);
//    printf("DATA=%.*s\r\n", data_len, data);

    char socket_type[10] = {0}, status[10] = {0};
    int onoff = 0;

    if (_check_ha_state(topic, topic_len, data, data_len) == ESP_OK)
        return;
    
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
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("main", socket_main_sta), 0, 1, 0);        
            sla_relay_main_ctrl(socket_main_sta);
            
            if (0 == socket_main_sta && 1 == socket_sub_sta) {
                vTaskDelay(10);
                esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("sub", 0), 0, 1, 0);
                socket_sub_sta = 0;
                sla_relay_sub_ctrl(socket_sub_sta);
                sla_led_w_ctrl(socket_sub_sta);        
            }
        }

        if (0 != socket_type[0] && !strcmp(socket_type, "sub")) {
            //sub relay ctrl            

            socket_sub_sta = onoff;

            if (0 == socket_main_sta) {
                esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("sub", 0), 0, 1, 0);
                socket_sub_sta = 0;
            } else {
                esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("sub", socket_sub_sta), 0, 1, 0);
            }
            sla_relay_sub_ctrl(socket_sub_sta);
            sla_led_w_ctrl(socket_sub_sta);        
        }

        if (0 != socket_type[0] && !strcmp(socket_type, "ota")) {
            //ota enable/disable

            if (!strcmp(status, "Enable")) {
                esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("ota", 1), 0, 1, 0);
                socket_ota_sta = 1;
                _sla_web_ota();
            } else if (!strcmp(status, "Disable")) {
                esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("ota", 0), 0, 1, 0);
                socket_ota_sta = 0;
                _sla_web_ota();
            }          
        }
    } else if (!strncmp(topic, mqtt_topic_get, topic_len)) {
        if (0 != socket_type[0] && !strcmp(socket_type, "main")) {
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("main", socket_main_sta), 0, 1, 0);
        }

        if (0 != socket_type[0] && !strcmp(socket_type, "sub")) {
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("sub", socket_sub_sta), 0, 1, 0);
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
            ESP_LOGW(TAG, "MQTT_EVENT_CONNECTED");
            is_cloud_connected = 1;
            // add system cb
            _sla_sys_evt_cb(SYS_CLOUD_CONNECTED, NULL);

            sla_ha_mqtt_discovery(mqtt_object_id);
            
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("main", socket_main_sta), 0, 1, 0);
            vTaskDelay(10);
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("sub", socket_sub_sta), 0, 1, 0);
            vTaskDelay(10);
            esp_mqtt_client_publish(client, mqtt_topic_state, get_payload("ota", 0), 0, 1, 0);
            vTaskDelay(10);
            
            sla_ha_device_mqtt_publish_available();
            
            msg_id = esp_mqtt_client_subscribe(client, ha_mqtt_topic_state, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            
            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic_set, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, mqtt_topic_get, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGE(TAG, "MQTT_EVENT_DISCONNECTED");
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
    char mqtt_broker_ip[16] = {0};
    char mqtt_username[33] = {0};
    char mqtt_password[33] = {0};
    
    sla_get_mqtt_broker_params(mqtt_broker_ip, mqtt_object_id, mqtt_username, mqtt_password);
    _sla_prepare_topic(mqtt_object_id);
    sla_ha_device_mqtt_topic_not_available(mqtt_object_id, mqtt_topic_availability, mqtt_payload_not_available);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = mqtt_broker_ip,
        .username = mqtt_username,
        .password = mqtt_password,
        .lwt_topic = mqtt_topic_availability,
        .lwt_msg = mqtt_payload_not_available,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}

void sla_get_sy7t609_info_cb(const char payload[][30], int size)
{
    if (sla_is_cloud_connected()) {
        for (int i = 0; i < size; i++) {
            esp_mqtt_client_publish(mqtt_client, mqtt_topic_state, payload[i], 0, 1, 0);
            ESP_LOGI(TAG, "%s", payload[i]);
        }
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    sla_nvs_init();
    
    sla_led_init();
    
    sla_relay_init();

    sla_button_init(10000, _sla_button_cb);

    sla_sys_evt_cb_reg(_sla_sys_evt_cb);
    
    sla_sy7t609_init(sla_get_sy7t609_info_cb);
    
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
