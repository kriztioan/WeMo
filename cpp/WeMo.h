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
#include "Serial/Serial.h"
#include "Sun.h"
#include "log.h"
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

  WeMo();
  ~WeMo();

  bool ini_rescan();
  bool process();

  std::map<std::string, std::vector<WeMo::Timer>> timers;

  int fd_inotify;

  Serial serial;

  int lux_on;
  int lux_off;

  std::vector<Plug *> lux_control;

private:
  bool ini_read(const char *filename,
                std::map<std::string, std::map<std::string, std::string>> &ini);

  bool
  ini_parse(std::map<std::string, std::map<std::string, std::string>> &ini);

  int wd_inotify;

  float latitude;
  float longitude;
};

#endif
