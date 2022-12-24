/**
 *  @file   log.h
 *  @brief  Log Function Declaration
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
#include <cstring>
#include <ctime>

#include <sys/time.h>

int log(FILE *stream, const char *fmt, ...);
int logerror(const char *fmt, ...);

#endif