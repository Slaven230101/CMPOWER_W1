/**
  ******************************************************************************
  * @file       sla_relay_drv.h
  * @brief      Head file for slaven relay driver
  * @details 
  * @author     Slaven
  * @data       2024-05-17
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_RELAY_H__
#define __SLA_RELAY_H__
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sla_relay_init(void);

void sla_relay_main_ctrl(int onoff);

void sla_relay_sub_ctrl(int onoff);

void sla_relay_all_ctrl(int onoff);

void sla_relay_sep_ctrl(int onoff_main, int onoff_sub);

#ifdef __cplusplus
}
#endif

#endif
