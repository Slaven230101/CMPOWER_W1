/**
  ******************************************************************************
  * @file       sla_mqtt_disc.h
  * @brief      Head file for slaven MQTT discovery
  * @details 
  * @author     Slaven
  * @data       2024-07-20
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_MQTT_DISC_H__
#define __SLA_MQTT_DISC_H__
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void sla_ha_device_mqtt_topic_not_available(const char *object_id, char *mqtt_topic_availability, char *mqtt_payload_not_available);

void sla_ha_device_mqtt_publish_available(void);

esp_err_t sla_ha_mqtt_update_socket_main_config(const char *object_id, int flag);

esp_err_t sla_ha_mqtt_discovery(const char *object_id);

#ifdef __cplusplus
}
#endif

#endif
