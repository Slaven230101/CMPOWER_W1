/**
  ******************************************************************************
  * @file       sla_web_ota.h
  * @brief      Head file for slaven web ota
  * @details 
  * @author     Slaven
  * @data       2025-03-11
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_WEB_OTA_H__
#define __SLA_WEB_OTA_H__
#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t sla_web_ota_start(void);

esp_err_t sla_web_ota_stop(void);

#ifdef __cplusplus
}
#endif

#endif
