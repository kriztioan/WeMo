/**
 *  @file   wemo.cpp
 *  @brief  WeMo Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "WeMo.h"

WeMo::WeMo(const Settings &settings) {

  scan();

  load_settings(settings);
}

bool WeMo::load_settings(const Settings &settings) {

  this->settings = &settings;

  timers.clear();

  std::string name;

  char *m;

  time_t t;

  if (settings.find("global") != settings.end()) {

    if (settings["global"].find("rescan") != settings["global"].end()) {
      t = strtol(settings["global"]["rescan"].c_str(), NULL, 10);

      Log::info("Polling interval set to %d s", t);

      this->timers["rescan"].push_back((WeMo::Timer){
          .plug = NULL, .time = t, .action = "rescan", .name = "rescan"});

      rescan = time(NULL) + t;
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
  }

  return true;
}

void WeMo::check_lux(uint32_t lux) {

  static uint32_t lux_prev = 0;

  if (lux < lux_on && lux_prev >= lux_on) {

    for (std::vector<Plug *>::iterator it = lux_control.begin();
         it != lux_control.end(); it++) {

      (*it)->On();
    }
  } else if (lux > lux_off && lux_prev <= lux_off) {

    for (std::vector<Plug *>::iterator it = lux_control.begin();
         it != lux_control.end(); it++) {

      (*it)->Off();
    }
  }

  lux_prev = lux;
}

void WeMo::display_plugs() {
  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n"
          "                                   Plugs                       "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n"
          "Name                      State                                "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n");

  for (std::vector<Plug *>::iterator it = plugs.begin(); it != plugs.end();
       it++) {

    fprintf(Log::stream, "%-25s %-15s\n", (*it)->Name().c_str(),
            (*it)->State() ? "on" : "off");
  }

  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n");
}

void WeMo::display_lux() {
  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n"
          "                                 Lux Settings                  "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n"
          "On                        %-53d\n"
          "Off                       %-53d\n"
          "Controls                  ",
          lux_on, lux_off);

  int nchars = 0;

  if (lux_control.empty()) {

    nchars += fputc('-', Log::stream);
  }

  for (std::vector<Plug *>::iterator it = lux_control.begin();
       it != lux_control.end(); it++) {

    nchars += fprintf(Log::stream, "%s ", (*it)->Name().c_str());
  }

  for (int i = 0; i < (53 - nchars); i++) {

    fputc(' ', Log::stream);
  }
  fputc('\n', Log::stream);

  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n");
}

void WeMo::display_schedules() {

  display_schedule("daily");

  display_schedule("sun");
}

void WeMo::display_schedule(const char *schedule) {

  fprintf(Log::stream,
          "-----------------------------------------------------------"
          "----"
          "----------------\n"
          "                       Registered %s Timers and "
          "Schedule    "
          "                \n"
          "-----------------------------------------------------------"
          "----"
          "----------------\n"
          "Name                      Action         Date and time     "
          "    "
          "                \n"
          "-----------------------------------------------------------"
          "----"
          "----------------\n",
          schedule);

  std::vector<WeMo::Timer> timestamps;

  if (strcmp(schedule, "daily") == 0) {

    timestamps.push_back((WeMo::Timer){
        .plug = nullptr, .time = rescan, .action = "rescan", .name = "-"});
  }

  if (timers.find(schedule) != timers.end()) {

    time_t t, now = time(NULL);

    struct tm s_tm = *localtime(&now);
    s_tm.tm_sec = 0;
    s_tm.tm_min = 0;
    s_tm.tm_hour = 0;

    time_t today = mktime(&s_tm);

    for (std::vector<WeMo::Timer>::iterator it = timers[schedule].begin();
         it != timers[schedule].end(); it++) {

      t = today + (*it).time;

      if (now >= t) {

        t += (3600 * 24);
      }

      timestamps.push_back((WeMo::Timer){.plug = nullptr,
                                         .time = t,
                                         .action = (*it).action,
                                         .name = (*it).name});
    }

    std::sort(timestamps.begin(), timestamps.end(), WeMo::TimerCompare);
  }

  char date[64];

  for (std::vector<WeMo::Timer>::iterator it = timestamps.begin();
       it != timestamps.end(); it++) {

    strftime(date, sizeof(date), "%a, %d %B %Y %H:%M:%S",
             localtime(&(*it).time));

    fprintf(Log::stream, "%-25s %-15s %-37s\n", (*it).name.c_str(),
            (*it).action.c_str(), date);
  }
}

void WeMo::check_timer(const char *schedule) {

  if (timers.find(schedule) != timers.end()) {

    time_t t, now = time(NULL);

    struct tm s_tm = *localtime(&now);
    s_tm.tm_sec = 0;
    s_tm.tm_min = 0;
    s_tm.tm_hour = 0;

    time_t today = mktime(&s_tm);

    nearest = today + timers[schedule][0].time;

    if (now >= nearest) {

      nearest += (3600 * 24);
    }

    for (std::vector<WeMo::Timer>::iterator it = timers[schedule].begin();
         it != timers[schedule].end(); it++) {

      t = today + (*it).time;

      if (t >= now && t <= (now + 5)) {

        if ((*it).action == "on") {

          Log::info("Sending 'ON' to %s", (*it).name.c_str());
          (*it).plug->On();
        } else if ((*it).action == "off") {

          Log::info("Sending 'OFF' to %s", (*it).name.c_str());
          (*it).plug->Off();
        }

        t += (3600 * 24);
      }

      if (now >= t) {

        t += (3600 * 24);
      }

      if (t < nearest) {

        nearest = t;
      }
    }
  }
}

void WeMo::check_timers() {

  timerclear(&itimer.it_interval);

  timerclear(&itimer.it_value);

  time_t now = time(NULL);

  if (now >= rescan) {

    scan();

    load_settings(*settings);

    rescan = now + 3600;
    if (timers.find("rescan") != timers.end()) {

      rescan = now + timers["rescan"].begin()->time;
    }
  }
  nearest = rescan;

  check_timer("daily");

  time_t t = nearest;

  check_timer("sun");

  if (t < nearest) {

    nearest = t;
  }

  if (nearest > rescan) {

    nearest = rescan;
  }

  itimer.it_value.tv_sec = nearest - now;

  setitimer(ITIMER_REAL, &itimer, NULL);
}