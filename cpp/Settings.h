/**
 *  @file   Settings.h
 *  @brief  Settings Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-24
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <map>
#include <string>

#include <cstdio>

#include <sys/inotify.h>
#include <sys/stat.h>

#include "Log.h"

class Settings {
public:
  Settings() = delete;
  Settings(const std::string &filename);
  ~Settings();

  std::map<std::string, std::string> operator[](const std::string &key) const {
    return this->ini.at(key);
  }
  std::map<std::string, std::string> &operator[](const std::string &key) {
    return this->ini.at(key);
  }

  const std::map<std::string,
                 std::map<std::string, std::string>>::const_iterator
  end() const {
    return this->ini.end();
  }
  const std::map<std::string,
                 std::map<std::string, std::string>>::const_iterator
  find(const char *key) const {
    return ini.find(key);
  }

  int handler();

  int fd_inotify;

private:
  int parse();

  std::map<std::string, std::map<std::string, std::string>> ini;
  std::string filename;
  int wd_inotify;
};

#endif