/**
 *  @file   WeMo.h
 *  @brief  WeMo Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef WEMO_H_
#define WEMO_H_

#include <sstream>
#include <string>

#include <csignal>
#include <ctime>

#include <algorithm>
#include <map>
#include <vector>

#include "Discover.h"
#include "Log.h"
#include "Settings.h"
#include "Sun.h"

#include <sys/time.h>

#define TIME_T(x) (x & 0x00FFFFFF)
#define TIME_WD(x) (x >> 24)

class WeMo : public Discover {

public:
  typedef struct {
    Plug *plug;
    time_t time;
    std::string action;
  } Timer;

  inline static bool TimerCompare(WeMo::Timer &a, WeMo::Timer &b) {

    return (a.time < b.time);
  }

  WeMo() = delete;
  WeMo(const Settings &settings);
  ~WeMo() = default;

  bool load_settings(const Settings &settings);

  int check_timers();
  void check_lux(uint32_t lux);

  void display_plugs();
  void display_lux();
  void display_schedules();

private:
  time_t parse_time(const char *str, bool weekdays = true);
  time_t next_weekday(time_t wday);
  void check_schedule(const char *schedule);
  void display_schedule(const char *schedule);

  void poll();

  const Settings *settings;

  std::vector<Plug *> lux_control;
  std::map<std::string, std::vector<WeMo::Timer>> timers;

  struct itimerval itimer;

  time_t nearest_t;
  time_t poll_t;
  time_t trigger_t;
  time_t today_t;
  time_t weekday;

  float latitude;
  float longitude;

  int lux_on;
  int lux_off;
};

#endif
