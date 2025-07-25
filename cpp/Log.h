/**
 *  @file   Log.h
 *  @brief  Log Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-23
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef LOG_H_
#define LOG_H_

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <mutex>
#include <string>

#include <glob.h>
#include <regex.h>
#include <zlib.h>

#include <sys/time.h>
#include <unistd.h>

class Log {

public:
  static FILE *stream;

  Log() = delete;
  ~Log() = delete;

  static int init(const char *filename, long max_log_number = 0);
  static int close();

  static int info(const char *fmt, ...);
  static int warn(const char *fmt, ...);
  static int err(const char *fmt, ...);

  static int perror(const char *fmt, ...);

  static long keep(long max_log_number = 0);

private:
  enum Level { INFO, WARN, ERR };

  static std::string filename;

  static long max_log_number;

  static std::mutex mutex;

  static int log(Log::Level level, const char *fmt, ...);
  static int log(Log::Level level, const char *fmt, va_list ap);
  static int rotate();
};

#endif
