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
#include "Sensor.h"
#include "Settings.h"
#include "WeMo.h"

#include <cerrno>
#include <csignal>

#include <sys/signalfd.h>

int main(int argc, char *argv[], char **envp) {

  if (0 != Log::init("wemo.log")) {

    return errno;
  }

  Log::info("Starting WeMo daemon");

  Settings settings("wemo.ini");

  WeMo wemo(settings);

  Sensor sensor(settings);

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

  fd_set fd_in;

  wemo.check_timers();

  int finished = 0;

  while (!finished) {

    FD_ZERO(&fd_in);
    FD_SET(settings.fd_inotify, &fd_in);
    FD_SET(wemo.fd_socket, &fd_in);
    FD_SET(fd_signal, &fd_in);
    int fd_max =
        std::max(std::max(settings.fd_inotify, wemo.fd_socket), fd_signal);
    int fd_sensor = sensor.serial.filedescriptor();
    if (fd_sensor != -1) {

      FD_SET(fd_sensor, &fd_in);

      fd_max = std::max(fd_max, fd_sensor);
    }

    if (-1 == select(fd_max + 1, &fd_in, nullptr, nullptr, nullptr)) {

      Log::perror("Error in application select");

      finished = 1;
      break;
    }

    if (FD_ISSET(fd_sensor, &fd_in)) {

      wemo.check_lux(sensor.read());
    }

    if (FD_ISSET(settings.fd_inotify, &fd_in)) {

      if (settings.handler() == 0) {

        sensor.load_settings(settings);

        wemo.load_settings(settings);

        wemo.check_timers();
      }
    }

    if (FD_ISSET(fd_signal, &fd_in)) {

      struct signalfd_siginfo siginfo_s;

      if (0 > read(fd_signal, &siginfo_s, sizeof(siginfo_s))) {

        Log::perror("Error while reading signal");

        finished = 1;
        break;
      }

      if (siginfo_s.ssi_signo == SIGALRM || siginfo_s.ssi_signo == SIGUSR1) {

        wemo.check_timers();
      } else if (siginfo_s.ssi_signo == SIGUSR2) {

        wemo.display_plugs();

        sensor.display_sensor();

        wemo.display_lux();

        wemo.display_schedules();

        fflush(Log::stream);
      } else if (siginfo_s.ssi_signo == SIGINT ||
                 siginfo_s.ssi_signo == SIGQUIT ||
                 siginfo_s.ssi_signo == SIGTERM ||
                 siginfo_s.ssi_signo == SIGHUP) {

        finished = 1;
      }
    }

    if (FD_ISSET(wemo.fd_socket, &fd_in)) {

      wemo.message();
    }
  }

  close(fd_signal);

  Log::info("Stopped WeMo daemon");

  Log::close();

  return 0;
}
