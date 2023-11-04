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

  discover();

  load_settings(settings);
}

bool WeMo::load_settings(const Settings &settings) {

  this->settings = &settings;

  timers.clear();

  this->timers["poll"].push_back(
      (WeMo::Timer){.plug = NULL, .time = 3600, .action = "poll"});

  std::string name;

  char *m;

  time_t t;

  if (settings.find("global") != settings.end()) {

    if (settings["global"].find("poll") != settings["global"].end()) {

      static time_t tt = 3600;

      t = strtol(settings["global"]["poll"].c_str(), NULL, 10);

      if (t >= 60) {

        timers["poll"].begin()->time = t;
      } else {

        Log::warn("Minimal polling interval is 60 s, not setting %d s", t);

        t = tt;
      }

      if (t != tt) {

        Log::info("Polling interval changed to %d s", t);

        tt = t;
      }
    }

    if (settings["global"].find("latitude") != settings["global"].end()) {

      this->latitude = strtof(settings["global"]["latitude"].c_str(), nullptr);
    }

    if (settings["global"].find("longitude") != settings["global"].end()) {

      this->longitude =
          strtof(settings["global"]["longitude"].c_str(), nullptr);
    }
  }

  poll_t = time(NULL) + timers["poll"].begin()->time;

  Sun *sun = nullptr;

  for (std::vector<Plug>::iterator it = this->plugs.begin();
       it != this->plugs.end(); it++) {

    name = it->Name();

    if (settings.find(name.c_str()) != settings.end()) {

      if (settings[name].find("sun") != settings[name].end()) {

        if (settings[name]["sun"] == "true") {

          if (!sun) {

            sun = new Sun(latitude, longitude);
          }

          if ((t = parse_time(sun->rise().c_str())) != -1) {

            t += 15 * 60;

            this->timers["sun"].push_back(
                (WeMo::Timer){.plug = &(*it), .time = t, .action = "off"});
          } else {

            Log::warn("Failed to parse sun rise for '%s' ... ignoring",
                      name.c_str());
          }

          if ((t = parse_time(sun->set().c_str())) != -1) {

            t -= 30 * 60;

            this->timers["sun"].push_back(
                (WeMo::Timer){.plug = &(*it), .time = t, .action = "on"});
          } else {

            Log::warn("Failed to parse sun set for '%s' ... ignoring",
                      name.c_str());
          }
        }
      }

      if (settings[name].find("daily") != settings[name].end()) {

        if (settings[name]["daily"] == "true") {

          if (settings[name].find("ontimes") != settings[name].end()) {

            std::istringstream iss(settings[name]["ontimes"]);

            for (std::string token; std::getline(iss, token, ',');) {

              if ((t = parse_time(token.c_str())) != -1) {

                this->timers["daily"].push_back(
                    (WeMo::Timer){.plug = &(*it), .time = t, .action = "on"});
              } else {

                Log::warn("Failed to parse on time for '%s' ... ignoring",
                          name.c_str());
              }
            }
          }

          if (settings[name].find("offtimes") != settings[name].end()) {

            std::istringstream iss(settings[name]["offtimes"]);

            for (std::string token; std::getline(iss, token, ',');) {

              if ((t = parse_time(token.c_str())) != -1) {

                this->timers["daily"].push_back(
                    (WeMo::Timer){.plug = &(*it), .time = t, .action = "off"});
              } else {

                Log::warn("Failed to parse off time for '%s' ... ignoring",
                          name.c_str());
              }
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

        for (std::vector<Plug>::iterator it = this->plugs.begin();
             it != this->plugs.end(); it++) {

          if (it->name == token) {

            lux_control.push_back(&(*it));

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
          "Name                      State           Lost                 "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n");

  for (std::vector<Plug>::iterator it = plugs.begin(); it != plugs.end();
       it++) {

    fprintf(Log::stream, "%-25s %-15s %-38d\n", it->name.c_str(),
            it->lost      ? "?"
            : it->State() ? "on"
                          : "off",
            it->lost);
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

    nchars += fprintf(Log::stream, "%s ", (*it)->name.c_str());
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

  trigger_t = time(NULL);

  struct tm s_tm = *localtime(&trigger_t);
  s_tm.tm_sec = 0;
  s_tm.tm_min = 0;
  s_tm.tm_hour = 0;

  weekday = 1 << s_tm.tm_wday;

  today_t = mktime(&s_tm);

  display_schedule("daily");

  Sun sun(latitude, longitude);

  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n"
          "                                    Sun                        "
          "                \n"
          "---------------------------------------------------------------"
          "----------------\n"
          "Latitude                  %-53f\n"
          "Longitude                 %-53f\n"
          "Sun rise                  %-53s\n"
          "Sun set                   %-53s\n"
          "---------------------------------------------------------------"
          "----------------\n",
          latitude, longitude, sun.rise().c_str(), sun.set().c_str());

  if (timers["sun"].size()) {
    display_schedule("sun");
  }
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
          "Name                      Action          Date and time    "
          "    "
          "                \n"
          "-----------------------------------------------------------"
          "----"
          "----------------\n",
          schedule);

  std::vector<WeMo::Timer> timestamps;

  if (strcmp(schedule, "daily") == 0) {

    timestamps.push_back(
        (WeMo::Timer){.plug = nullptr, .time = poll_t, .action = "poll"});
  }

  if (timers.find(schedule) != timers.end()) {

    time_t t, wday;

    for (std::vector<WeMo::Timer>::iterator it = timers[schedule].begin();
         it != timers[schedule].end(); it++) {

      t = today_t + TIME_T(it->time);

      wday = TIME_WD(it->time);

      if (trigger_t >= t || (wday && !(weekday & wday))) {

        t += next_weekday(wday);
      }

      timestamps.push_back(
          (WeMo::Timer){.plug = it->plug, .time = t, .action = it->action});
    }

    std::sort(timestamps.begin(), timestamps.end(), WeMo::TimerCompare);
  }

  char date[64];

  for (std::vector<WeMo::Timer>::iterator it = timestamps.begin();
       it != timestamps.end(); it++) {

    strftime(date, sizeof(date), "%a, %B %d,%Y %H:%M:%S", localtime(&it->time));

    fprintf(Log::stream, "%-25s %-15s %-37s\n",
            it->plug ? it->plug->name.c_str() : "", it->action.c_str(), date);
  }

  fprintf(Log::stream,
          "---------------------------------------------------------------"
          "----------------\n");
}

void WeMo::poll() {

  std::vector<Plug> old = plugs;

  discover();

  for (std::vector<Plug>::iterator it = old.begin(); it != old.end(); it++) {

    std::vector<Plug>::iterator p =
        std::find_if(plugs.begin(), plugs.end(),
                     [&it](const Plug &p) { return it->ip == p.ip; });

    if (p == plugs.end()) {

      Log::info("Lost Plug at %s (%dx)", it->ip.c_str(), ++it->lost);

      if (it->lost < 5) {

        plugs.push_back(*it);
      } else {

        Log::info("De-registered Plug at %s", it->ip.c_str());
      }
    } else {

      p->lost = 0;
    }
  }

  load_settings(*settings);
}

void WeMo::check_schedule(const char *schedule) {

  if (timers.find(schedule) != timers.end()) {

    time_t t = TIME_T(timers[schedule][0].time),
           wday = TIME_WD(timers[schedule][0].time);

    nearest_t = today_t + t;

    if (trigger_t >= nearest_t) {

      nearest_t += next_weekday(wday);
    }

    for (std::vector<WeMo::Timer>::iterator it = timers[schedule].begin();
         it != timers[schedule].end(); it++) {

      t = today_t + TIME_T(it->time);

      wday = TIME_WD(it->time);

      if ((!wday || (wday && (weekday & wday))) && t >= trigger_t &&
          t <= (trigger_t + 3)) {

        if (it->action == "on") {

          Log::info("Sending 'ON' to %s", it->plug->name.c_str());
          it->plug->On();
        } else if (it->action == "off") {

          Log::info("Sending 'OFF' to %s", it->plug->name.c_str());
          it->plug->Off();
        }

        t += next_weekday(wday);
      }

      if (trigger_t >= t || (wday && !(weekday & wday))) {

        t += next_weekday(wday);
      }

      if (t < nearest_t) {

        nearest_t = t;
      }
    }
  }
}

int WeMo::check_timers() {

  struct timeval t_val;
  if (-1 == gettimeofday(&t_val, NULL)) {

    Log::perror("Failed to get time of day");

    return errno;
  }
  trigger_t = t_val.tv_sec;

  timerclear(&itimer.it_interval);

  timerclear(&itimer.it_value);

  if (-1 == setitimer(ITIMER_REAL, &itimer, NULL)) {

    Log::perror("Failed to clear timer");

    return errno;
  }

  struct tm s_tm = *localtime(&trigger_t);
  s_tm.tm_sec = 0;
  s_tm.tm_min = 0;
  s_tm.tm_hour = 0;
  today_t = mktime(&s_tm);

  weekday = 1 << s_tm.tm_wday;

  if (trigger_t >= poll_t) {

    poll();
  }

  nearest_t = poll_t;

  check_schedule("daily");

  time_t t = nearest_t;

  check_schedule("sun");

  if (t < nearest_t) {

    nearest_t = t;
  }

  if (nearest_t > poll_t) {

    nearest_t = poll_t;
  }

  if (-1 == gettimeofday(&t_val, NULL)) {

    Log::perror("Failed to get time of day");

    return errno;
  }

  char date[64];

  strftime(date, sizeof(date), "%a, %B %d, %Y at %H:%M:%S",
           localtime(&nearest_t));

  Log::info("Setting timer for %s", date);

  itimer.it_value = {nearest_t - t_val.tv_sec - 1, 1000000 - t_val.tv_usec};
  if (-1 == setitimer(ITIMER_REAL, &itimer, NULL)) {

    Log::perror("Failed to set timer");

    return errno;
  }

  return 0;
}

time_t WeMo::parse_time(const char *str) {

  char *m, days_of_week = 0;

  time_t t, l = strtol(str, &m, 10);
  if (l < 0 || l > 23) {

    Log::warn("Invalid value in '%s': %ld", str, l);
    return -1;
  }

  t = 3600 * l;

  if (*m == ':') {

    l = strtol(++m, &m, 10);
    if (l < 0 || l > 59) {

      Log::warn("Invalid value in '%s': %ld", str, l);
      return -1;
    }

    t += 60 * l;

    if (*m == ':') {

      l = strtol(++m, &m, 10);
      if (l < 0 || l > 59) {

        Log::warn("Invalid value in '%s': %ld", str, l);
        return -1;
      }

      t += l;
    }
  }

  if (*m++ == '%') {
    if (*m == '*') {
      days_of_week = (char)0b01111111;
    } else {

      bool has_range = false;
      char start_char = 0, end_char = 0;

      while (*m) {
        if (isdigit(*m)) {
          if (*m < '1' || *m > '7') {

            Log::warn("Invalid value in '%s': %c", str, *m);
            return -1;
          }

          if (has_range) {
            if (*m < start_char) {

              Log::warn("Invalid value in '%s': %c", str, *m);
              return -1;
            } else {
              end_char = *m++;
            }
          } else {
            start_char = *m++;
          }
        } else if (start_char && *m++ == '-') {
          has_range = true;
          continue;
        } else {

          Log::warn("Invalid value in '%s': %c", str, *m);
          return -1;
        }

        do {
          days_of_week |= 1 << (start_char - '1');
        } while (start_char++ < end_char);

        if (end_char) {
          end_char = 0;
          has_range = false;
        }
      }
    }
  }

  return t | (((time_t)days_of_week) << 24);
}

time_t WeMo::next_weekday(time_t wday) {

  time_t t = 0;

  int i = 0;
  do {

    ++i;

    t += (3600 * 24);
  } while (wday && !((((weekday << i) & 0x7F) | (weekday >> (7 - i))) & wday));

  return t;
}
