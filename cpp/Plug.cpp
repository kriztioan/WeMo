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

  if (this->lost) {

    Log::warn("Plug lost at %s: using last known name '%s'", this->ip.c_str(),
              this->name.c_str());

    return this->name;
  }

  this->name = name.length() > 0 ? this->SOAPRequest("SetFriendlyName", name)
                                 : this->SOAPRequest("GetFriendlyName");

  return this->name;
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

    Log::err("Invalid SOAP request: %s", service.c_str());

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

  int fd_socket = -1;
  if (-1 == (fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))) {

    Log::perror("Failed to create TCP socket");

    close(fd_socket);

    return "";
  }

  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_port = htons(this->port);
  if (1 != inet_pton(remote.sin_family, this->ip.c_str(), &remote.sin_addr)) {

    Log::perror("Failed to set socket address");

    close(fd_socket);

    return "";
  }
  memset(&remote.sin_zero, '\0', 8);

  int yes = 1;
  if (-1 ==
      setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {

    Log::perror("Failed to set socket reuse address");
    return "";
  }

  if (-1 ==
      connect(fd_socket, (struct sockaddr *)&remote, sizeof(struct sockaddr))) {

    Log::perror("Failed to connect socket for SOAP request");

    close(fd_socket);

    return "";
  }

  std::string msg = "POST /upnp/control/basicevent1 HTTP/1.1\r\n"
                    "Host: " +
                    this->ip + ":" + std::to_string(ntohs(this->port)) +
                    "\r\n"
                    "User-Agent: WeMo-daemon/1.0\r\n"
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

    if ((sent = send(fd_socket, msg_p + sent, bytes, 0)) <= 0) {

      Log::perror("Failed to send SOAP request");

      close(fd_socket);

      return "";
    }

    bytes -= sent;
  }

  std::string response;

  char buff[2048];

  while ((bytes = recv(fd_socket, buff, sizeof(buff) - 1, 0)) > 0) {

    buff[bytes] = '\0';

    response.append(buff, bytes);
  }

  close(fd_socket);

  if (bytes == -1) {

    Log::perror("Error while receiving SOAP response");

    return "";
  }

  size_t s = response.find(response_tag), e = response.find("</", s);

  return response.substr(s + response_tag.length(),
                         e - s - response_tag.length());
}
