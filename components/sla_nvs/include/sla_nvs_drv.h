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

esp_err_t sla_nvs_str_read(const char *key, char *data, int *p_len);

esp_err_t sla_nvs_str_write(const char *key, char *data);

esp_err_t sla_nvs_int_read(const char *key, int32_t *out_value);

esp_err_t sla_nvs_int_write(const char *key, int32_t value);

#ifdef __cplusplus
}
#endif

#endif
