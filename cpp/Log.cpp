/**
 *  @file   log.cpp
 *  @brief  Log Class Implementations
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-23
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Log.h"

std::string Log::filename;

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

  Log::filename = filename;

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

int Log::log(Log::Level level, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int size = Log::log(level, fmt, ap);
  va_end(ap);
  return size;
}

int Log::info(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::INFO, fmt, args);

  va_end(args);

  size += fputc('\n', Log::stream);

  fflush(Log::stream);

  Log::rotate();

  return size;
}

int Log::warn(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::WARN, fmt, args);

  va_end(args);

  size += fputc('\n', stream);

  fflush(Log::stream);

  Log::rotate();

  return size;
}

int Log::err(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::ERR, fmt, args);

  va_end(args);

  size += fputc('\n', Log::stream);

  fflush(Log::stream);

  Log::rotate();

  return size;
}

int Log::perror(const char *fmt, ...) {

  va_list(args);

  va_start(args, fmt);

  int size = Log::log(Log::Level::ERR, fmt, args);

  va_end(args);

  size += fprintf(Log::stream, ": %s\n", strerror(errno));

  fflush(Log::stream);

  Log::rotate();

  return size;
}

int Log::rotate() {

  if (ftell(Log::stream) < 200) { //1L << 20) {

    return 0;
  }

  glob_t logs;

  std::string ptrn = Log::filename + ".*.gz";

  int res;
  if ((res = glob(ptrn.c_str(), 0, NULL, &logs)) != 0) {
    if (res != GLOB_NOMATCH) {
      Log::log(Log::Level::ERR, "glob: %s\n", strerror(errno));
      return errno;
    }
  }

  ptrn = Log::filename;

  const char *escape = "\\+?*.[]()", *p = escape;
  while (*(p++)) {
    std::string::size_type beg = 0, end;
    while ((end = ptrn.find(*p, beg)) != std::string::npos) {
      ptrn.insert(end, "\\");
      beg = end + 2;
    }
  }
  ptrn += "\\.\\([1-9][0-9]*\\)\\.gz";

  regex_t reegex;
  if (regcomp(&reegex, ptrn.c_str(), 0) != 0) {
    Log::log(Log::Level::ERR, "Failed to compile regular expression\n");
    globfree(&logs);
    return -1;
  }

  size_t nmatch = 2;
  regmatch_t pmatch[nmatch];

  long max_number = 0;
  for (int i = 0; i < logs.gl_pathc; i++) {
    char *fp = logs.gl_pathv[i];
    if (regexec(&reegex, fp, nmatch, pmatch, 0) != 0) {

      continue;
    }

    fp[pmatch[1].rm_eo] = '\0';
    long number = strtol(fp + pmatch[1].rm_so, NULL, 10);
    if (number > max_number) {
      max_number = number;
    }
  }
  ++max_number;

  regfree(&reegex);

  globfree(&logs);

  printf("%ld\n", max_number);

  std::string gzfile = Log::filename + "." + std::to_string(max_number) + ".gz";

  struct gzFile_s *gz = gzopen(gzfile.c_str(), "wb9");
  if (NULL == gz) {
    Log::log(Log::Level::ERR, "Failed to open gz-file: %s\n", strerror(errno));
    return errno;
  }

  if (-1 == fseek(Log::stream, 0, SEEK_SET)) {
    Log::log(Log::Level::ERR, "Failed to fseek: %s\n", strerror(errno));
    return errno;
  }

  char buff[4096];
  while (0 == feof(Log::stream)) {
    if (0 == gzwrite(gz, buff, fread(buff, 1, sizeof(buff), Log::stream))) {
      Log::log(Log::Level::ERR, "Failed to write to gz-file\n");
      return -1;
    }
  }

  gzclose(gz);

  if (0 != ftruncate(fileno(Log::stream), 0)) {
    Log::log(Log::Level::ERR, "Failed to truncate log file: %s\n",
             strerror(errno));
    return errno;
  }

  if (-1 == fseek(Log::stream, 0, SEEK_SET)) {
    Log::log(Log::Level::ERR, "Failed to fseek: %s\n", strerror(errno));
    return errno;
  }

  Log::info("Successfully rotated log");

  return 1;
}