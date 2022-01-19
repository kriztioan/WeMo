/**
 *  @file   Plug.cpp
 *  @brief  Plug Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-17
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Plug.h"

Plug::Plug(std::string ip, int port) : ip(ip), port(port) {}

std::string Plug::Name(std::string name) {

  if (name.length() > 0) {

    return this->SOAPRequest("SetFriendlyName", name);
  }

  return this->SOAPRequest("GetFriendlyName");
}

bool Plug::isOn() { return this->State() == Plug::ON; }

bool Plug::isOff() { return this->State() == Plug::OFF; }

bool Plug::State() { return this->SOAPRequest("GetBinaryState") == "1"; }

bool Plug::Toggle() {

  if (this->isOn()) {

    return this->Off() == Plug::OFF;
  }

  return this->On() == Plug::ON;
}

bool Plug::On() {

  if (this->isOff()) {

    return this->SOAPRequest("SetBinaryState", "1") == "1";
  }

  return true;
}

bool Plug::Off() {

  if (this->isOn()) {

    return this->SOAPRequest("SetBinaryState", "0") == "0";
  }

  return true;
}

std::string Plug::SOAPRequest(std::string service, std::string arg) {

  std::string param, response_tag;

  if (service == "GetFriendlyName") {

    response_tag = "<FriendlyName>";
  } else if (service == "SetFriendlyName") {

    response_tag = "<FriendlyName>";

    param = response_tag + arg + "</FriendlyName>";
  } else if (service == "GetBinaryState") {

    response_tag = "<BinaryState>";
  } else if (service == "SetBinaryState") {

    response_tag = "<BinaryState>";

    param = response_tag + arg + "</BinaryState>";
  } else {

    return "";
  }

  std::string xml =
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
      "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
      "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
      "<s:Body>"
      "<u:" +
      service + " xmlns:u=\"urn:Belkin:service:basicevent:1\">" + param +
      "</u:" + service +
      ">"
      "</s:Body>"
      "</s:Envelope>\r\n";

  struct protoent *protocol = NULL;
  if (NULL == (protocol = getprotobyname("tcp"))) {

    return "";
  }

  int sockfd = -1;
  if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, protocol->p_proto))) {
    perror("socket");
    return "";
  }

  int yes = 1;
  if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {

    perror("setsockopt");
    return ("");
  }

  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_port = htons(this->port);
  if (1 != inet_pton(remote.sin_family, this->ip.c_str(), &remote.sin_addr)) {

    perror("inet_pton");
    return ("");
  }
  memset(&remote.sin_zero, '\0', 8);

  if (-1 ==
      connect(sockfd, (struct sockaddr *)&remote, sizeof(struct sockaddr))) {

    perror("connect");
    return ("");
  }

  std::string msg = "POST /upnp/control/basicevent1 HTTP/1.1\r\n"
                    "Host: " +
                    this->ip + ":" + std::to_string(ntohs(this->port)) +
                    "\r\n"
                    "User-Agent: WeMo-deamon/1.0\r\n"
                    "Content-Type: text/xml; charset=\"utf-8\"\r\n"
                    "Content-Length: " +
                    std::to_string(xml.size()) +
                    "\r\n"
                    "Accept: application/xml\r\n"
                    "SOAPAction: \"urn:Belkin:service:basicevent:1#" +
                    service +
                    "\"\r\n"
                    "Connection: close\r\n"
                    "\r\n" +
                    xml;

  const char *msg_p = msg.c_str();
  ssize_t sent = 0, bytes = msg.size();

  while (bytes > 0) {

    if (-1 == (sent = send(sockfd, msg_p + sent, bytes, 0))) {

      perror("sent");
      return ("");
    }
    bytes -= sent;
  }

  std::string response;

  char buff[2048];

  while ((bytes = recv(sockfd, buff, sizeof(buff) - 1, 0))) {

    buff[bytes] = '\0';

    response.append(buff, bytes);
  }

  close(sockfd);

  if (bytes == -1) {

    perror("recv");
    return ("");
  }

  size_t s = response.find(response_tag), e = response.find("</", s);

  return response.substr(s + response_tag.length(),
                         e - s - response_tag.length());
}
