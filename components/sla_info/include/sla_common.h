/**
  ******************************************************************************
  * @file       sla_common.h
  * @brief      Head file for slaven common
  * @details 
  * @author     Slaven
  * @data       2025-03-18
  * @version    V1.0
  * @copyright  Slaven
  *
  ******************************************************************************
**/

#ifndef __SLA_COMMON_H__
#define __SLA_COMMON_H__
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

void get_firmware_build_date(char *s);

char* get_firmware_version(void);

#ifdef __cplusplus
}
#endif

#endif
