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

#include <fstream>
#include <map>
#include <string>

#include <cstdio>

#include <sys/inotify.h>
#include <sys/stat.h>

#include <openssl/evp.h>
#include <openssl/md5.h>

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
  begin() const {
    return this->ini.begin();
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
  const std::map<std::string,
                 std::map<std::string, std::string>>::const_iterator
  find(const std::string &key) const {
    return ini.find(key);
  }

  int handler();

  int fd_inotify;

private:
  int parse();
  int md5sum();

  std::map<std::string, std::map<std::string, std::string>> ini;
  std::string filename;
  int wd_inotify;

  unsigned char md5[MD5_DIGEST_LENGTH] = {};
};

#endif
