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

#define BROKER_IP_MAX_LEN   15
#define OBJECT_ID_MAX_LEN   32
#define USERNAME_MAX_LEN    32
#define PASSWORD_MAX_LEN    32

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

esp_err_t sla_wifi_mac_get(uint8_t *mac);

void sla_smartconfig_init(void);

int sla_is_wifi_config(void);

int sla_is_wifi_connected(void);

char* sla_get_sta_ip(void* event_data);

void sla_wifi_reset(void);

void sla_get_mqtt_broker_params(char *ip, char *user_topic, char *username, char *password);

#ifdef __cplusplus
}
#endif
