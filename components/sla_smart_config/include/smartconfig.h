/**
  ******************************************************************************
  * @file       smartconfig.h
  * @brief      Head file for slaven smartconfig
  * @details 
  * @author     Slaven
  * @data       2024-05-28
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define BROKER_URL_MAX_LEN  64
#define USER_TOPIC_MAX_LEN  32

typedef enum {
    SYS_EVENT_NONE,
    SYS_WIFI_UNCONFIG,
    SYS_WIFI_CONFIG,
    SYS_WIFI_CONNECTED,
    SYS_WIFI_DISCONNECT,
    SYS_CLOUD_CONNECTED,
    SYS_CLOUD_DISCONNECTED,
} sys_type_t;

esp_err_t sla_sys_evt_cb_reg(esp_err_t (*fun)(sys_type_t, void*));

void sla_smartconfig_init(void);

int sla_is_wifi_config(void);

int sla_is_wifi_connected(void);

void sla_wifi_reset(void);

void sla_get_mqtt_broker_url_and_user_topic(char *url, char *user_topic);

#ifdef __cplusplus
}
#endif
