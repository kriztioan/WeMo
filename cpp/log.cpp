/**
 *  @file   log.cpp
 *  @brief  Log Class Implementations
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-23
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Log.h"

FILE *Log::stream = NULL;

int Log::init(const char *filename) {

  if (NULL == (Log::stream = freopen(filename, "a+", stdout))) {

    Log::perror("Failed to redirect stdout to file");

    return (errno);
  }

  if (-1 == dup2(STDOUT_FILENO, STDERR_FILENO)) {

    Log::perror("Failed to duplicate stdout to stderr");

    return (errno);
  }

  return 0;
}

int Log::close() {

  if (Log::stream != NULL) {
    fclose(Log::stream);
    return 0;
  }

  return 1;
}
int Log::log(Log::Level level, const char *fmt, va_list ap) {

  struct timeval timeval_s;

  gettimeofday(&timeval_s, NULL);

  struct tm *tm_info = localtime(&timeval_s.tv_sec);

  char buff[20];

  strftime(buff, sizeof(buff), "%Y-%d-%mT%T", tm_info);

  int size = fprintf(Log::stream, "%s.%06d ", buff, timeval_s.tv_usec);

  switch (level) {
  case Log::Level::INFO:
    size += fprintf(Log::stream, "[INFO] ");
    break;
  case Log::Level::WARN:
    size += fprintf(Log::stream, "[WARN] ");
    break;
  case Log::Level::ERR:
    size += fprintf(Log::stream, "[ERR]  ");
    break;
  }

  size += vfprintf(Log::stream, fmt, ap);

  return size;
}

int Log::info(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::INFO, fmt, args);

  va_end(args);

  size += fputc('\n', stream);

  fflush(Log::stream);

  return size;
}

int Log::warn(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::WARN, fmt, args);

  va_end(args);

  size += fputc('\n', stream);

  fflush(Log::stream);

  return size;
}

int Log::err(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::ERR, fmt, args);

  va_end(args);

  size += fprintf(stream, "\n");

  fflush(Log::stream);

  return size;
}

int Log::perror(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::ERR, fmt, args);

  va_end(args);

  size += fprintf(Log::stream, ": %s\n", strerror(errno));

  fflush(Log::stream);

  return size;
}
