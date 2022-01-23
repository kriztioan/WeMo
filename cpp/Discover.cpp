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

Discover::Discover() { scan(); }

bool Discover::scan() {

  destroy();

  if (-1 == (this->sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))) {

    perror("socket");
    return false;
  }

  struct ip_mreq mc_req;
  mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
  mc_req.imr_multiaddr.s_addr = inet_addr(Discover::ADDRESS);

  if (-1 == setsockopt(this->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mc_req,
                       sizeof(mc_req))) {

    perror("setsockopt");
    return false;
  }

  if (!this->broadcast()) {

    return false;
  }

  if (!this->receive()) {

    return false;
  }

  if (-1 == setsockopt(this->sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mc_req,
                       sizeof(mc_req))) {

    perror("setsockopt");
    return false;
  }

  close(this->sockfd);

  return true;
}

void Discover::destroy() {

  for (std::vector<Plug *>::iterator it = this->plugs.begin();
       it != this->plugs.end(); it++) {

    delete *it;
  }
  this->plugs.clear();
}

bool Discover::broadcast() {

  struct sockaddr_in mc;

  mc.sin_family = AF_INET;
  mc.sin_port = htons(Discover::PORT);
  if (0 == inet_aton(Discover::ADDRESS, &mc.sin_addr)) {

    perror("inet_aton");
    return false;
  }
  memset(&mc.sin_zero, '\0', 8);

  std::string msg = "M-SEARCH * HTTP/1.1\r\n"
                    "HOST: " +
                    std::string(Discover::ADDRESS) + ":" +
                    std::to_string(Discover::PORT) +
                    "\r\n"
                    "MAN: \"ssdp:discover\"\r\n"
                    "MX: 5\r\n"
                    //"ST: ssdp:all\r\n"
                    "ST: urn:Belkin:device:controllee:1\r\n"
                    "\r\n";

  if (-1 == sendto(this->sockfd, msg.c_str(), msg.size(), 0,
                   (struct sockaddr *)&mc, sizeof(struct sockaddr))) {

    perror("sendto");
  }

  return true;
}

bool Discover::receive() {

  struct timeval rcvtimeo;
  rcvtimeo.tv_sec = 3;
  rcvtimeo.tv_usec = 0;

  if (-1 == setsockopt(this->sockfd, SOL_SOCKET, SO_RCVTIMEO,
                       (const char *)&rcvtimeo, sizeof(rcvtimeo))) {

    perror("setsockopt");
    return false;
  }

  int reuseaddr = 1;
  if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                       sizeof(reuseaddr))) {

    perror("setsockopt");
    return false;
  }

  struct sockaddr_in from;
  socklen_t from_size = sizeof(struct sockaddr);
  char buff[2048];
  time_t timeout = time(NULL) + 3;
  ssize_t bytes;

  while (time(NULL) < timeout) {

    memset(&from, 0, from_size);

    if (-1 == (bytes = recvfrom(this->sockfd, buff, sizeof(buff), 0,
                                (struct sockaddr *)&from, &from_size))) {

      if(!(errno == EAGAIN || errno == EWOULDBLOCK))
        perror("recvfrom");
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

            this->plugs.push_back(new Plug(ip, atoi(port)));
          }
        }
      }
    }
  }

  return true;
}
