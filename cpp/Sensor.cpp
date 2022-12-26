/**
 *  @file   Sensor.h
 *  @brief  Sensor Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-26
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Sensor.h"

Sensor::Sensor(const Settings &settings) { this->load_settings(settings); }

bool Sensor::load_settings(const Settings &settings) {

  if (settings.find("serial") != settings.end()) {

    if (settings["serial"].find("port") != settings["serial"].end()) {

      if (settings["serial"].find("baudrate") != settings["serial"].end()) {

        int baudrate =
            strtol(settings["serial"]["baudrate"].c_str(), nullptr, 10);

        if (!serial.good()) {

          serial.open(settings["serial"]["port"], baudrate);
        } else if (serial.port() != settings["serial"]["port"] ||
                   serial.baudrate() != baudrate) {

          serial.close();

          serial.open(settings["serial"]["port"], baudrate);
        }
      } else {

        if (!serial.good()) {

          serial.open(settings["serial"]["port"]);
        } else if (serial.port() != settings["serial"]["port"]) {

          serial.close();

          serial.open(settings["serial"]["port"]);
        }
      }
    }
  } else {

    serial.close();
  }

  return serial.good();
}

uint32_t Sensor::read() {

  uint32_t lux;

  ssize_t nbytes = this->serial.read(&lux, sizeof(lux));

  if (nbytes <= 0) {

    Log::err("Failed to read from serial device: %s (%d)",
             this->serial.getErrorMessage(), this->serial.getErrorCode());

    return -1;
  }

  return lux;
}

void Sensor::display_sensor() {
  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n"
          "                                   Sensor                      "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n"
          "Status                    %-53s\n"
          "Port                      %-53s\n"
          "---------------------------------------------------------------"
          "----------------\n",
          this->serial.good() ? "detected" : "not detected",
          serial.port().c_str());
}