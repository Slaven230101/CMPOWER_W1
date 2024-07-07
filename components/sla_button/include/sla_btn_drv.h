/**
  ******************************************************************************
  * @file       sla_btn_drv.h
  * @brief      Head file for slaven button driver
  * @details 
  * @author     Slaven
  * @data       2024-05-15
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_BTN_H__
#define __SLA_BTN_H__
#include "esp_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
   DRV_BUTTON_PRESS_SHORT,
   DRV_BUTTON_PRESS_RELEASE,
   DRV_BUTTON_PRESS_HOLD,
} drv_btn_type_t;

esp_err_t sla_button_init(uint32_t long_press_ms, void (*btn_cb)(drv_btn_type_t, uint32_t));

#ifdef __cplusplus
}
#endif

#endif
