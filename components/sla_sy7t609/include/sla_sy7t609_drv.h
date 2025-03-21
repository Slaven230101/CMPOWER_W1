/**
  ******************************************************************************
  * @file       sla_sy7t609_drv.h
  * @brief      Head file for slaven SY7T609 driver
  * @details 
  * @author     Slaven
  * @data       2024-07-23
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_SY7T609_DRV_H__
#define __SLA_SY7T609_DRV_H__
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sla_sy7t609_init(void (*fun)(const char payload[][30], int size));

#ifdef __cplusplus
}
#endif

#endif