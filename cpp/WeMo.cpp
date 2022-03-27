/**
 *  @file   WeMo.cpp
 *  @brief  WeMo Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "WeMo.h"

static constexpr char ini_file[] = "wemo.ini";

WeMo::WeMo() {

  if (!process()) {

    return;
  }

  if (0 > (this->fd_inotify = inotify_init())) {

    perror("inotify_init");
    return;
  }

  if (-1 == (this->wd_inotify =
                 inotify_add_watch(this->fd_inotify, ini_file, IN_MODIFY))) {

    perror("inotify_add_watch");
  }
}

bool WeMo::process() {

  this->timers.clear();

  std::map<std::string, std::map<std::string, std::string>> ini;

  if (!ini_read(ini_file, ini)) {

    return false;
  }

  if (!ini_parse(ini)) {

    return false;
  }

  return true;
}

WeMo::~WeMo() {

  if (0 < this->fd_inotify) {

    inotify_rm_watch(this->fd_inotify, this->wd_inotify);

    close(this->fd_inotify);
  }
}

bool WeMo::ini_rescan() {

  ssize_t buf_size = 128 * sizeof(struct inotify_event), i = 0;

  char buf[buf_size];

  ssize_t len;

  if ((len = read(this->fd_inotify, buf, buf_size)) < 0) {
    perror("read");
    return false;
  }

  bool rescan = false;

  while (i < len) {

    struct inotify_event *e = (struct inotify_event *)(buf + i);

    if (e->mask & IN_MODIFY)
      rescan = true;
    i += (sizeof(struct inotify_event) + e->len);
  }

  if (rescan) {

    return process();
  }

  return true;
}

bool WeMo::ini_read(
    const char *filename,
    std::map<std::string, std::map<std::string, std::string>> &ini) {

  struct stat st;
  if (0 != stat("wemo.ini", &st)) {
    perror("stat");
    return false;
  }

  size_t n = 64;

  char *l = (char *)malloc(n), s[64], k[64], v[64];

  FILE *f = NULL;
  if (NULL == (f = fopen("wemo.ini", "r"))) {

    perror("fopen");
    return false;
  }

  while (getline(&l, &n, f) != -1) {

    if (*l == '\n' || *l == '#') {

      continue;
    }

    if (sscanf(l, "[%[^]]", s) == 1) {

      continue;
    }

    if (sscanf(l, "%[^=]=%s ", k, v) == 2) {

      ini[s][k] = v;
    }
  }

  free(l);

  fclose(f);

  return true;
}

bool WeMo::ini_parse(
    std::map<std::string, std::map<std::string, std::string>> &ini) {

  std::string name;

  char *m;

  time_t t;

  if (ini.find("global") != ini.end()) {

    if (ini["global"].find("rescan") != ini["global"].end()) {

      this->timers["rescan"].push_back((WeMo::Timer){
          .plug = NULL,
          .time = strtol(ini["global"]["rescan"].c_str(), NULL, 10),
          .action = "rescan",
          .name = "rescan"});
    }

    if (ini["global"].find("latitude") != ini["global"].end()) {

      this->latitude = strtof(ini["global"]["latitude"].c_str(), nullptr);
    }

    if (ini["global"].find("longitude") != ini["global"].end()) {

      this->longitude = strtof(ini["global"]["longitude"].c_str(), nullptr);
    }
  }

  Sun *sun = nullptr;

  for (std::vector<Plug *>::iterator it = this->plugs.begin();
       it != this->plugs.end(); it++) {

    name = (*it)->Name();

    if (ini.find(name) != ini.end()) {

      if (ini[name].find("sun") != ini[name].end()) {

        if (ini[name]["sun"] == "true") {

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
      
      if (ini[name].find("daily") != ini[name].end()) {

        if (ini[name]["daily"] == "true") {

          if (ini[name].find("ontimes") != ini[name].end()) {

            std::istringstream iss(ini[name]["ontimes"]);

            for (std::string token; std::getline(iss, token, ',');) {

              t = 3600 * strtol(token.c_str(), &m, 10);

              if (*m == ':') {

                t += 60 * strtol(++m, NULL, 10);
              }

              this->timers["daily"].push_back((WeMo::Timer){
                  .plug = *it, .time = t, .action = "on", .name = name});
            }
          }

          if (ini[name].find("offtimes") != ini[name].end()) {

            std::istringstream iss(ini[name]["offtimes"]);

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

  if (ini.find("serial") != ini.end()) {

    if (ini["serial"].find("port") != ini["serial"].end()) {

      if (ini["serial"].find("baudrate") != ini["serial"].end()) {

        int baudrate = strtol(ini["serial"]["baudrate"].c_str(), nullptr, 10);

        if (!serial.good()) {

          serial.open(ini["serial"]["port"], baudrate);
        } else if (serial.port() != ini["serial"]["port"] ||
                   serial.baudrate() != baudrate) {

          serial.close();

          serial.open(ini["serial"]["port"], baudrate);
        }
      } else {

        if (!serial.good()) {

          serial.open(ini["serial"]["port"]);
        } else if (serial.port() != ini["serial"]["port"]) {

          serial.close();

          serial.open(ini["serial"]["port"]);
        }
      }
    }

    if (ini["serial"].find("onlux") != ini["serial"].end()) {

      lux_on = strtol(ini["serial"]["onlux"].c_str(), nullptr, 10);
    }

    if (ini["serial"].find("offlux") != ini["serial"].end()) {

      lux_off = strtol(ini["serial"]["offlux"].c_str(), nullptr, 10);
    }

    if (ini["serial"].find("controls") != ini["serial"].end()) {

      std::istringstream iss(ini["serial"]["controls"]);

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
