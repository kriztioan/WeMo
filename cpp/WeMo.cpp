/**
 *  @file   WeMo.cpp
 *  @brief  WeMo Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "WeMo.h"

WeMo::WeMo(const Settings &settings) {

  scan();

  process(settings);
}

bool WeMo::process(const Settings &settings) {

  this->timers.clear();

  if (!parse_settings(settings)) {

    return false;
  }

  return true;
}

bool WeMo::parse_settings(const Settings &settings) {

  std::string name;

  char *m;

  time_t t;

  if (settings.find("global") != settings.end()) {

    if (settings["global"].find("rescan") != settings["global"].end()) {
      long t = strtol(settings["global"]["rescan"].c_str(), NULL, 10);

      Log::info("Polling interval set to %d s", t);

      this->timers["rescan"].push_back((WeMo::Timer){
          .plug = NULL, .time = t, .action = "rescan", .name = "rescan"});
    }

    if (settings["global"].find("latitude") != settings["global"].end()) {

      this->latitude = strtof(settings["global"]["latitude"].c_str(), nullptr);
    }

    if (settings["global"].find("longitude") != settings["global"].end()) {

      this->longitude =
          strtof(settings["global"]["longitude"].c_str(), nullptr);
    }
  }

  Sun *sun = nullptr;

  for (std::vector<Plug *>::iterator it = this->plugs.begin();
       it != this->plugs.end(); it++) {

    name = (*it)->Name();

    if (settings.find(name.c_str()) != settings.end()) {

      if (settings[name].find("sun") != settings[name].end()) {

        if (settings[name]["sun"] == "true") {

          if (!sun) {

            sun = new Sun(latitude, longitude);
          }

          t = 3600 * strtol(sun->rise().c_str(), &m, 10);

          if (*m == ':') {

            t += 60 * strtol(++m, NULL, 10);
          }

          t += 15 * 60;

          this->timers["sun"].push_back((WeMo::Timer){
              .plug = *it, .time = t, .action = "off", .name = name});

          t = 3600 * strtol(sun->set().c_str(), &m, 10);

          if (*m == ':') {

            t += 60 * strtol(++m, NULL, 10);
          }

          t -= 30 * 60;

          this->timers["sun"].push_back((WeMo::Timer){
              .plug = *it, .time = t, .action = "on", .name = name});
        }
      }

      if (settings[name].find("daily") != settings[name].end()) {

        if (settings[name]["daily"] == "true") {

          if (settings[name].find("ontimes") != settings[name].end()) {

            std::istringstream iss(settings[name]["ontimes"]);

            for (std::string token; std::getline(iss, token, ',');) {

              t = 3600 * strtol(token.c_str(), &m, 10);

              if (*m == ':') {

                t += 60 * strtol(++m, NULL, 10);
              }

              this->timers["daily"].push_back((WeMo::Timer){
                  .plug = *it, .time = t, .action = "on", .name = name});
            }
          }

          if (settings[name].find("offtimes") != settings[name].end()) {

            std::istringstream iss(settings[name]["offtimes"]);

            for (std::string token; std::getline(iss, token, ',');) {

              t = 3600 * strtol(token.c_str(), &m, 10);

              if (*m == ':') {

                t += 60 * strtol(++m, NULL, 10);
              }

              this->timers["daily"].push_back((WeMo::Timer){
                  .plug = *it, .time = t, .action = "off", .name = name});
            }
          }
        }
      }
    }
  }

  if (sun) {

    delete sun;
  }

  lux_control.clear();

  lux_on = lux_off = -1;

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

    if (settings["serial"].find("onlux") != settings["serial"].end()) {

      lux_on = strtol(settings["serial"]["onlux"].c_str(), nullptr, 10);
    }

    if (settings["serial"].find("offlux") != settings["serial"].end()) {

      lux_off = strtol(settings["serial"]["offlux"].c_str(), nullptr, 10);
    }

    if (settings["serial"].find("controls") != settings["serial"].end()) {

      std::istringstream iss(settings["serial"]["controls"]);

      for (std::string token; std::getline(iss, token, ',');) {

        for (std::vector<Plug *>::iterator it = this->plugs.begin();
             it != this->plugs.end(); it++) {

          if ((*it)->Name() == token) {

            lux_control.push_back(*it);

            break;
          }
        }
      }
    }
  } else if (serial.good()) {

    serial.close();
  }

  return true;
}
