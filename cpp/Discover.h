/**
 *  @file   Discover.h
 *  @brief  Discover Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef DISCOVER_H_
#define DISCOVER_H_

#include <cstring>
#include <ctime>

#include <string>
#include <vector>

#include "Plug.h"
#include "Log.h"

class Discover {

public:
  Discover();
  ~Discover();

  bool scan();
  void message();

  std::vector<Plug *> plugs;

  int fd_multicast;

private:
  static const char *ADDRESS;

  static const int PORT = 1900;

  struct ip_mreq mc_req;

  bool broadcast();
  bool receive();

  void destroy();
};

#endif
