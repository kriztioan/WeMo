/**
 *  @file   Discover.cpp
 *  @brief  Discover Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Discover.h"

const char *Discover::ADDRESS = "239.255.255.250";

bool Discover::setup() {

  if (-1 == (this->fd_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {

    Log::perror("Failed to open UDP socket");

    return false;
  }

  const int yes = 1, no = 0;

  struct in_addr addr_if;

  struct timeval rcvtimeo;
  rcvtimeo.tv_sec = 1;
  rcvtimeo.tv_usec = 0;
  if (-1 == setsockopt(this->fd_socket, SOL_SOCKET, SO_RCVTIMEO,
                       (const char *)&rcvtimeo, sizeof(rcvtimeo))) {

    Log::perror("Failed to set socket timeout");

    goto FAIL;
  }

  if (-1 ==
      setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {

    Log::perror("Failed to set socket reuse address");

    goto FAIL;
  }

  if (-1 ==
      setsockopt(fd_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes))) {

    Log::perror("Failed to set socket reuse port");

    goto FAIL;
  }

  if (-1 == 
      setsockopt(fd_socket, IPPROTO_IP, IP_MULTICAST_LOOP, &no, sizeof(no))) {

    Log::perror("Failed to set multicast loop");

    goto FAIL;
  }

  addr_if.s_addr = INADDR_ANY;
  if (setsockopt(fd_socket, IPPROTO_IP, IP_MULTICAST_IF, &addr_if, sizeof(addr_if))) {

    Log::perror("Failed setting multicast interface");

    goto FAIL;
  }

  return true;

FAIL:
  close(this->fd_socket);
  this->fd_socket = -1;
  return false;
}

Discover::~Discover() {

  if(this->fd_socket != -1) {

    close(this->fd_socket);
  }
}

bool Discover::discover() {

  if (this->fd_socket == -1) {

    if(!this->setup()) {

      Log::err("Failed to set up socket");

      return false;
    }
  }

  Log::info("Scanning for Plugs");

  destroy();

  if (!this->broadcast()) {

    return false;
  }

  return this->receive();
}

void Discover::destroy() { this->plugs.clear(); }

bool Discover::broadcast() {

  struct sockaddr_in mc;

  std::string msg = "M-SEARCH * HTTP/1.1\r\n"
                    "HOST: " +
                    std::string(Discover::ADDRESS) + ":" +
                    std::to_string(Discover::PORT) +
                    "\r\n"
                    "MAN: \"ssdp:discover\"\r\n"
                    "MX: " + std::to_string(this->mx) +"\r\n"
                    //"ST: ssdp:all\r\n"
                    "ST: urn:Belkin:device:controllee:1\r\n"
                    "USER-AGENT: WeMo Daemon\r\n"
                    "\r\n";

  mc.sin_family = AF_INET;
  mc.sin_port = htons(Discover::PORT);
  if (0 == inet_aton(Discover::ADDRESS, &mc.sin_addr)) {

    Log::perror("Failed to set multicast broadcast address");
  
    goto FAIL;
  }
  memset(&mc.sin_zero, '\0', 8);


  if (-1 == sendto(this->fd_socket, msg.c_str(), msg.size(), 0,
                   (struct sockaddr *)&mc, sizeof(struct sockaddr))) {

    Log::perror("Failed to send multicast message");
  
    goto FAIL;
  }

  if (0 == port) {
    socklen_t len = sizeof(struct sockaddr);
    if (-1 == getsockname(fd_socket, (sockaddr *)&mc, &len)) {

      Log::perror("Failed to get UDP port");

      goto FAIL;
    }

    port = ntohs(mc.sin_port);

    Log::info("Receiving UDP on port %d", port);
  }

  return true;

FAIL:
  close(this->fd_socket);
  this->fd_socket = -1;
  return false;
}

bool Discover::receive() {

  struct sockaddr_in from;
  socklen_t from_size = sizeof(struct sockaddr);
  char buff[2048];
  time_t timeout = time(NULL) + this->mx;
  ssize_t bytes;

  while (time(NULL) < timeout) {

    memset(&from, 0, from_size);

    if (-1 == (bytes = recvfrom(this->fd_socket, buff, sizeof(buff), 0,
                                (struct sockaddr *)&from, &from_size))) {

      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {

        Log::perror("Error while receiving response");
      }

      continue;
    }

    char *location = NULL;

    if (bytes > 0) {

      buff[bytes] = '\0';

      if ((location = strstr(buff, "LOCATION: ")) != NULL) {

        location += 10;

        char *ip = NULL;
        if ((ip = strstr(location, "http://")) != NULL) {

          ip += 7;

          char *port = NULL;
          if ((port = strchr(ip, ':')) != NULL) {

            *port++ = '\0';

            int p = atoi(port);

            Log::info("Found Plug at %s:%d", ip, p);

            this->plugs.emplace_back(ip, p);
          }
        }
      }
    }
  }

  return true;
}

void Discover::message() {

  struct sockaddr_in from;
  socklen_t from_size = sizeof(struct sockaddr);
  char buff[2048];
  ssize_t bytes;

  Log::info("RECEIVING MESSAGE");

  while ((bytes = recvfrom(this->fd_socket, buff, sizeof(buff), 0,
                           (struct sockaddr *)&from, &from_size)) > 0) {

    buff[bytes] = '\0';

    fprintf(Log::stream, "%s", buff);
    fflush(Log::stream);
  }
  fputc('\n', Log::stream);
  fflush(Log::stream);
}