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

#include "mgos.h"
#include "mgos_sht31_internal.h"
#include "mgos_i2c.h"

// Datasheet:
// https://cdn-shop.adafruit.com/product-files/2857/Sensirion_Humidity_SHT3x_Datasheet_digital-767294.pdf

// Private functions follow
static bool mgos_sht31_cmd(struct mgos_sht31 *sensor, uint16_t cmd) {
  uint8_t data[2];

  if (!sensor || !sensor->i2c) {
    return false;
  }

  data[0] = cmd >> 8;
  data[1] = cmd & 0xFF;
  if (!mgos_i2c_write(sensor->i2c, sensor->i2caddr, data, 2, true)) {
    LOG(LL_ERROR, ("I2C=0x%02x cmd=%u (0x%04x) write error", sensor->i2caddr, cmd, cmd));
    return false;
  }
  LOG(LL_DEBUG, ("I2C=0x%02x cmd=%u (0x%04x) write success", sensor->i2caddr, cmd, cmd));

  return true;
}

static uint8_t crc8(const uint8_t *data, int len) {
  const uint8_t poly = 0x31;
  uint8_t       crc  = 0xFF;

  for (int j = len; j; --j) {
    crc ^= *data++;
    for (int i = 8; i; --i) {
      crc = (crc & 0x80) ? (crc << 1) ^ poly : (crc << 1);
    }
  }
  return crc;
}

static uint16_t mgos_sht31_status(struct mgos_sht31 *sensor) {
  uint8_t  data[3];
  uint16_t value;

  mgos_sht31_cmd(sensor, MGOS_SHT31_READSTATUS);
  if (!mgos_i2c_read(sensor->i2c, sensor->i2caddr, data, 3, true)) {
    return 0;
  }

  // Check CRC8 checksums
  if ((data[2] != crc8(data, 2))) {
    return 0;
  }

  value = (data[0] << 8) + data[1];

  return value;
}

// Private functions end

// Public functions follow
struct mgos_sht31 *mgos_sht31_create(struct mgos_i2c *i2c, uint8_t i2caddr) {
  struct mgos_sht31 *sensor;
  uint16_t           status0, status1, status2;

  if (!i2c) {
    return NULL;
  }

  sensor = calloc(1, sizeof(struct mgos_sht31));
  if (!sensor) {
    return NULL;
  }

  memset(sensor, 0, sizeof(struct mgos_sht31));
  sensor->i2caddr = i2caddr;
  sensor->i2c     = i2c;

  mgos_sht31_cmd(sensor, MGOS_SHT31_SOFTRESET);

  // Toggle heater on and off, which shows up in status register bit 13 (0=Off, 1=On)
  status0 = mgos_sht31_status(sensor); // heater is off, bit13 is 0
  mgos_sht31_cmd(sensor, MGOS_SHT31_HEATEREN);
  status1 = mgos_sht31_status(sensor); // heater is on, bit13 is 1
  mgos_sht31_cmd(sensor, MGOS_SHT31_HEATERDIS);
  status2 = mgos_sht31_status(sensor); // heater is off, bit13 is 0

  if (((status0 & 0x2000) == 0) && ((status1 & 0x2000) != 0) && ((status2 & 0x2000) == 0)) {
    LOG(LL_INFO, ("SHT31 created at I2C 0x%02x", i2caddr));
    return sensor;
  }

  LOG(LL_ERROR, ("Failed to create SHT31 at I2C 0x%02x", i2caddr));
  free(sensor);
  return NULL;
}

void mgos_sht31_destroy(struct mgos_sht31 **sensor) {
  if (!*sensor) {
    return;
  }

  free(*sensor);
  *sensor = NULL;
  return;
}

bool mgos_sht31_read(struct mgos_sht31 *sensor) {
  double start = mg_time();

  if (!sensor || !sensor->i2c) {
    return false;
  }

  sensor->stats.read++;

  if (start - sensor->stats.last_read_time < MGOS_SHT31_READ_DELAY) {
    sensor->stats.read_success_cached++;
    return true;
  }
  // Read out sensor data here
  //
  uint8_t data[6];
  float   humidity, temperature;

  mgos_sht31_cmd(sensor, MGOS_SHT31_MEAS_HIGHREP);

  mgos_usleep(15000);

  if (!mgos_i2c_read(sensor->i2c, sensor->i2caddr, data, 6, true)) {
    return false;
  }

  // Check CRC8 checksums
  if ((data[2] != crc8(data, 2)) || (data[5] != crc8(data + 3, 2))) {
    return false;
  }

  temperature         = data[0] * 256 + data[1];
  temperature        *= 175;
  temperature        /= 0xffff;
  temperature        -= 45;
  sensor->temperature = temperature;

  humidity         = data[3] * 256 + data[4];
  humidity        *= 100;
  humidity        /= 0xFFFF;
  sensor->humidity = humidity;

  LOG(LL_DEBUG, ("temperature=%.2fC humidity=%.1f%%", sensor->temperature, sensor->humidity));
  sensor->stats.read_success++;
  sensor->stats.read_success_usecs += 1000000 * (mg_time() - start);
  sensor->stats.last_read_time      = start;
  return true;
}

float mgos_sht31_getTemperature(struct mgos_sht31 *sensor) {
  if (!mgos_sht31_read(sensor)) {
    return NAN;
  }

  return sensor->temperature;
}

float mgos_sht31_getHumidity(struct mgos_sht31 *sensor) {
  if (!mgos_sht31_read(sensor)) {
    return NAN;
  }

  return sensor->humidity;
}

bool mgos_sht31_getStats(struct mgos_sht31 *sensor, struct mgos_sht31_stats *stats) {
  if (!sensor || !stats) {
    return false;
  }

  memcpy((void *)stats, (const void *)&sensor->stats, sizeof(struct mgos_sht31_stats));
  return true;
}

bool mgos_sht31_i2c_init(void) {
  return true;
}

// Public functions end
