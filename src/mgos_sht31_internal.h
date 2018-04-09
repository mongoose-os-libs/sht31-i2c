/*
 * Copyright 2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "mgos.h"
#include "mgos_i2c.h"
#include "mgos_sht31.h"
#include <math.h>

#define MGOS_SHT31_DEFAULT_I2CADDR         (0x44)

#define MGOS_SHT31_MEAS_HIGHREP_STRETCH    (0x2C06)
#define MGOS_SHT31_MEAS_MEDREP_STRETCH     (0x2C0D)
#define MGOS_SHT31_MEAS_LOWREP_STRETCH     (0x2C10)
#define MGOS_SHT31_MEAS_HIGHREP            (0x2400)
#define MGOS_SHT31_MEAS_MEDREP             (0x240B)
#define MGOS_SHT31_MEAS_LOWREP             (0x2416)
#define MGOS_SHT31_READSTATUS              (0xF32D)
#define MGOS_SHT31_CLEARSTATUS             (0x3041)
#define MGOS_SHT31_SOFTRESET               (0x30A2)
#define MGOS_SHT31_HEATEREN                (0x306D)
#define MGOS_SHT31_HEATERDIS               (0x3066)

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_sht31 {
  struct mgos_i2c *       i2c;
  uint8_t                 i2caddr;
  struct mgos_sht31_stats stats;

  float                   humidity, temperature;
};

#ifdef __cplusplus
}
#endif
