/**
 *  @file   log.cpp
 *  @brief  Log Function Implementations
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-23
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "log.h"

int log(FILE *stream, const char *fmt, ...) {

  struct timeval timeval_s;

  gettimeofday(&timeval_s, NULL);

  struct tm *tm_info = localtime(&timeval_s.tv_sec);

  char buff[20];

  strftime(buff, sizeof(buff), "%Y-%d-%mT%T", tm_info);

  int size = fprintf(stream, "%s.%06d ", buff, timeval_s.tv_usec);

  va_list(args);

  va_start(args, fmt);

  size += vfprintf(stream, fmt, args);

  va_end(args);

  fflush(stream);

  return size;
}

int logerror(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = log(stderr, fmt, args);

  va_end(args);

  size += fprintf(stderr, ": %s\n", strerror(errno));

  return size;
}