/**
  ******************************************************************************
  * @file       sla_nvs_drv.h
  * @brief      Head file for slaven nvs driver
  * @details 
  * @author     Slaven
  * @data       2024-05-15
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_NVS_DRV_H__
#define __SLA_NVS_DRV_H__
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sla_nvs_init(void);

esp_err_t sla_nvs_read(const char *key, char *data, int *p_len);

esp_err_t sla_nvs_write(const char *key, char *data);

#ifdef __cplusplus
}
#endif

#endif
