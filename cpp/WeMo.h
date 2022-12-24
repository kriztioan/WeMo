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

#include <sys/inotify.h>
#include <sys/stat.h>

#include <map>
#include <vector>

#include "Discover.h"
#include "Log.h"
#include "Serial/Serial.h"
#include "Sun.h"
#include "Settings.h"

class WeMo : public Discover {

public:
  typedef struct {
    Plug *plug;
    time_t time;
    std::string action;
    std::string name;
  } Timer;

  inline static bool TimerCompare(WeMo::Timer &a, WeMo::Timer &b) {

    return (a.time < b.time);
  }

  WeMo() = delete;
  WeMo(const Settings &settings);
  ~WeMo() = default;

  bool process(const Settings &settings);

  std::map<std::string, std::vector<WeMo::Timer>> timers;

  Serial serial;

  int lux_on;
  int lux_off;

  std::vector<Plug *> lux_control;

private:

  bool
  parse_settings(const Settings &settings);

  int wd_inotify;

  float latitude;
  float longitude;
};

#endif
