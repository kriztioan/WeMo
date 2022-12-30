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

#include "Log.h"
#include "Plug.h"

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

  int port = 0;

  bool broadcast();
  bool receive();

  void destroy();
};

#endif
