/**
  ******************************************************************************
  * @file       sla_led_drv.h
  * @brief      Head file for slaven led driver
  * @details 
  * @author     Slaven
  * @data       2024-05-17
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_LED_H__
#define __SLA_LED_H__
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sla_led_init(void);

void sla_led_r_ctrl(int onoff);

void sla_led_w_ctrl(int onoff);

void sla_led_b_ctrl(int onoff);

#ifdef __cplusplus
}
#endif

#endif
