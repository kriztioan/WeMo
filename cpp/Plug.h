/**
 *  @file   Plug.cpp
 *  @brief  Plug Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef PLUG_H_
#define PLUG_H_

#include <cstring>

#include <string>

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "log.h"
class Plug {

public:
  static const int OFF = 0;
  static const int ON = 1;

  Plug(std::string ip, int port);

  std::string Name(std::string name = "");

  bool isOn();
  bool isOff();
  bool State();
  bool Toggle();

  bool On();
  bool Off();

private:
  std::string SOAPRequest(std::string service, std::string arg = "");

  std::string ip;
  int port;
};

#endif
