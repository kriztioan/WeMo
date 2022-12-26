/**
 *  @file   Sensor.h
 *  @brief  Sensor Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-26
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef SENSOR_H_
#define SENSOR_H_

#include "Log.h"
#include "Serial/Serial.h"
#include "Settings.h"

class Sensor {
public:
  Sensor() = delete;
  Sensor(const Settings &settings);
  ~Sensor() = default;

  bool load_settings(const Settings &settings);

  void display_sensor();

  uint32_t read();

  Serial serial;
};
#endif