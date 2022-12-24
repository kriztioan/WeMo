/**
 *  @file   main.cpp
 *  @brief  WeMo Mini Smart Plug Control Daemon
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 *  Compile and run with:
 *    make
 *    ./wemod &
 *
 ***********************************************/

#include "Log.h"
#include "Settings.h"
#include "WeMo.h"

#include <algorithm>

#include <cerrno>
#include <csignal>
#include <cstdarg>

#include <sys/signalfd.h>
#include <sys/time.h>

Settings settings("wemo.ini");

WeMo *wemo;

time_t now, today, nearest, last, t, rescan;

struct itimerval itimer;

void _checktimers(const char *type, int fire = 0) {

  if (wemo->timers.find(type) != wemo->timers.end()) {

    nearest = today + wemo->timers[type][0].time;

    if (now >= nearest) {

      nearest += (3600 * 24);
    }

    for (std::vector<WeMo::Timer>::iterator it = wemo->timers[type].begin();
         it != wemo->timers[type].end(); it++) {

      t = today + (*it).time;

      if (fire > 0 && t > last && t < now + 5) {

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

void checktimers(int fire = 0) {

  nearest = rescan;

  _checktimers("daily", fire);

  size_t t = nearest;

  _checktimers("sun", fire);

  if (t < nearest) {

    nearest = t;
  }

  if (nearest > rescan) {

    nearest = rescan;
  }

  last = now + 5;

  itimer.it_value.tv_sec = nearest - now;

  setitimer(ITIMER_REAL, &itimer, nullptr);
}

void scan() {

  itimer.it_value.tv_sec = 0;
  setitimer(ITIMER_REAL, &itimer, nullptr);

  if (wemo->scan() && wemo->process(settings)) {

    rescan = now + 3600;

    if (wemo->timers.find("rescan") != wemo->timers.end()) {

      rescan = now + wemo->timers["rescan"].begin()->time;
    }

    checktimers();
  }
}

void display_schedule(const char *type) {

  fprintf(Log::stream,
          "-----------------------------------------------------------"
          "----"
          "----------------\n"
          "                       registered %s timers and "
          "schedule    "
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
          type);

  std::vector<WeMo::Timer> timestamps;

  if (strcmp(type, "daily") == 0) {

    timestamps.push_back((WeMo::Timer){
        .plug = nullptr, .time = rescan, .action = "rescan", .name = "-"});
  }

  if (wemo->timers.find(type) != wemo->timers.end()) {

    for (std::vector<WeMo::Timer>::iterator it = wemo->timers[type].begin();
         it != wemo->timers[type].end(); it++) {

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

int main(int argc, char *argv[], char **envp) {

  if (0 != Log::init("wemo.log")) {

    return errno;
  }

  Log::info("Starting WeMo daemon");

  sigset_t s_set;
  sigemptyset(&s_set);

  sigaddset(&s_set, SIGALRM);
  sigaddset(&s_set, SIGINT);
  sigaddset(&s_set, SIGQUIT);
  sigaddset(&s_set, SIGTERM);
  sigaddset(&s_set, SIGHUP);
  sigaddset(&s_set, SIGUSR1);
  sigaddset(&s_set, SIGUSR2);

  sigprocmask(SIG_BLOCK, &s_set, nullptr);

  int fd_signal = signalfd(-1, &s_set, SFD_NONBLOCK);

  now = time(nullptr);

  last = now;

  struct tm s_tm = *localtime(&now);
  s_tm.tm_sec = 0;
  s_tm.tm_min = 0;
  s_tm.tm_hour = 0;

  today = mktime(&s_tm);

  timerclear(&itimer.it_interval);
  timerclear(&itimer.it_value);

  wemo = new WeMo(settings);

  rescan = now + 3600;
  if (wemo->timers.find("rescan") != wemo->timers.end()) {

    rescan = now + wemo->timers["rescan"].begin()->time;
  }

  checktimers();

  fd_set fd_in;

  int finished = 0;

  while (!finished) {

    FD_ZERO(&fd_in);
    FD_SET(settings.fd_inotify, &fd_in);
    FD_SET(wemo->fd_multicast, &fd_in);
    FD_SET(fd_signal, &fd_in);
    int fd_max =
        std::max(std::max(settings.fd_inotify, wemo->fd_multicast), fd_signal);

    int fd_serial = wemo->serial.filedescriptor();

    if (fd_serial != -1) {

      FD_SET(fd_serial, &fd_in);

      fd_max = std::max(fd_max, fd_serial);
    }

    if (-1 == select(fd_max + 1, &fd_in, nullptr, nullptr, nullptr)) {

      Log::perror("Error in application loop (select)");

      finished = 1;
      break;
    }

    if (FD_ISSET(fd_serial, &fd_in)) {

      static int lux_prev = -1;

      uint32_t lux;

      ssize_t nbytes = wemo->serial.read(&lux, sizeof(lux));

      if (nbytes <= 0) {

        fprintf(
            Log::stream,
            "=====================================ERROR====================="
            "===="
            "================\n"
            "serial: %s (%d)\n"
            "==============================================================="
            "================\n",
            wemo->serial.getErrorMessage(), wemo->serial.getErrorCode());

        continue;
      }

      if (nbytes) {

        if (wemo->lux_control.empty()) {

          continue;
        }

        if (lux_prev < 0) {

          lux_prev = lux;

          continue;
        }

        if (lux < wemo->lux_on && lux_prev >= wemo->lux_on) {

          for (std::vector<Plug *>::iterator it = wemo->lux_control.begin();
               it != wemo->lux_control.end(); it++) {

            (*it)->On();
          }
        } else if (lux > wemo->lux_off && lux_prev <= wemo->lux_off) {

          for (std::vector<Plug *>::iterator it = wemo->lux_control.begin();
               it != wemo->lux_control.end(); it++) {

            (*it)->Off();
          }
        }

        lux_prev = lux;
      }
    }

    if (FD_ISSET(wemo->fd_multicast, &fd_in)) {

      wemo->message();
    }

    if (FD_ISSET(settings.fd_inotify, &fd_in)) {

      if (settings.handler() == 0) {

        if (wemo->process(settings)) {

          rescan = now + 3600;

          if (wemo->timers.find("rescan") != wemo->timers.end()) {

            rescan = now + wemo->timers["rescan"].begin()->time;
          }

          checktimers();
        }
      }
    }

    if (FD_ISSET(fd_signal, &fd_in)) {

      now = time(nullptr);

      s_tm = *localtime(&now);
      s_tm.tm_sec = 0;
      s_tm.tm_min = 0;
      s_tm.tm_hour = 0;

      today = mktime(&s_tm);

      struct signalfd_siginfo siginfo_s;

      if (0 > read(fd_signal, &siginfo_s, sizeof(siginfo_s))) {

        Log::perror("Error while reading signal");

        finished = 1;
        break;
      }

      if (siginfo_s.ssi_signo == SIGALRM) {

        if (now >= rescan) {

          scan();
        }

        checktimers(1);
      } else if (siginfo_s.ssi_signo == SIGUSR1) {

        scan();

        checktimers();
      } else if (siginfo_s.ssi_signo == SIGUSR2) {

        Log::info("SCHEDULE:");

        fprintf(
            Log::stream,
            "---------------------------------------------------------------"
            "----------------\n"
            "                         %.24s                           \n"
            "==============================================================="
            "================\n"
            "                                    WeMo                       "
            "                \n"
            "==============================================================="
            "================\n"
            "                               plugs detected                  "
            "                \n"
            "---------------------------------------------------------------"
            "----------------\n"
            "Name                      State                                "
            "                \n"
            "---------------------------------------------------------------"
            "----------------\n",
            ctime(&now));

        for (std::vector<Plug *>::iterator it = wemo->plugs.begin();
             it != wemo->plugs.end(); it++) {

          fprintf(Log::stream, "%-25s %-15s\n", (*it)->Name().c_str(),
                  (*it)->State() ? "on" : "off");
        }

        fprintf(
            Log::stream,
            "---------------------------------------------------------------"
            "----------------\n"
            "                                   serial                      "
            "                \n"
            "---------------------------------------------------------------"
            "----------------\n"
            "Status                    %-53s\n"
            "Port                      %-53s\n"
            "On                        %-53d\n"
            "Off                       %-53d\n"
            "Controls                  ",
            fd_serial > -1 ? "detected" : "not detected",
            wemo->serial.port().c_str(), wemo->lux_on, wemo->lux_off);

        int nchars = 0;

        if (wemo->lux_control.empty()) {

          nchars += fprintf(Log::stream, "%c", '-');
        }

        for (std::vector<Plug *>::iterator it = wemo->lux_control.begin();
             it != wemo->lux_control.end(); it++) {

          nchars += fprintf(Log::stream, "%s ", (*it)->Name().c_str());
        }

        for (int i = 0; i < (53 - nchars); i++) {

          fputc(' ', Log::stream);
        }
        fputc('\n', Log::stream);

        display_schedule("daily");

        display_schedule("sun");

        fprintf(
            Log::stream,
            "==============================================================="
            "================\n");

        fflush(Log::stream);
      } else if (siginfo_s.ssi_signo == SIGINT ||
                 siginfo_s.ssi_signo == SIGQUIT ||
                 siginfo_s.ssi_signo == SIGTERM ||
                 siginfo_s.ssi_signo == SIGHUP) {

        finished = 1;
      }
    }
  }

  close(fd_signal);

  Log::info("Stopped WeMo deamon");

  Log::close();

  delete wemo;

  return 0;
}
