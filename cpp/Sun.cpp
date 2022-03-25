/**
 *  @file   Sun.cpp
 *  @brief  Sun Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-03-25
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Sun.h"

Sun::Sun(float latitude, float longitude, int tz_offset) {

  time_t t = time(NULL);

  int fd;

  const char *file = "sun.cache";
  if (access(file, F_OK) == 0) {
    fd = open(file, O_RDONLY);
    read(fd, &cache, sizeof(cache));
    close(fd);
    if (cache.latitude == latitude && cache.longitude == longitude &&
        cache.tz_offset == tz_offset && cache.expires < t) {

      return;
    }
  }

  cache.latitude = latitude;
  cache.longitude = longitude;
  cache.tz_offset = tz_offset;
  cache.expires = t + 3600 * 8;
  *cache.rise = '\0';
  *cache.set = '\0';

  std::vector<std::string> headers = {"Accept: application/json"};

  std::stringstream url;
  url << "api.sunrise-sunset.org/json?lat=" << latitude << "&lng=" << longitude;

  std::string json = https_get(url.str(), headers);

  rapidjson::Document rapid;
  if (!rapid.Parse(json.c_str()).HasParseError()) {
    if (strncmp(rapid["status"].GetString(), "OK", 2) == 0) {

      strncpy(cache.rise,
              utc_to_local(rapid["results"]["sunrise"].GetString(), tz_offset)
                  .c_str(),
              5);
      strncpy(cache.set,
              utc_to_local(rapid["results"]["sunset"].GetString(), tz_offset)
                  .c_str(),
              5);
    }
  }

  fd = open(file, O_WRONLY | O_CREAT, 0644);
  write(fd, &cache, sizeof(cache));
  close(fd);
}

std::string Sun::utc_to_local(const std::string &utc, int tz_offset) {

  time_t t = time(NULL);

  struct tm s_tm = *localtime(&t);

  tz_offset += s_tm.tm_isdst;

  char *err;
  if ((err = strptime(utc.c_str(), "%l:%M:%S %p", &s_tm)) != NULL && !*err) {

    s_tm.tm_hour += tz_offset;

    std::stringstream ss;

    ss << (((s_tm.tm_hour %= 24) < 0) ? s_tm.tm_hour + 24 : s_tm.tm_hour) << ':'
       << std::setw(2) << std::setfill('0') << s_tm.tm_min;

    return ss.str();
  }

  return std::string();
}

std::string Sun::https_get(std::string url, std::vector<std::string> headers,
                           short port, size_t block_size) {

  if (url.empty()) {
    return (std::string());
  }

  std::string path(url, url.find_first_of('/'),
                   url.size() - url.find_first_of('/')),
      hostname(url, 0, url.find_first_of('/'));

  SSL_library_init();

  std::stringstream ss;
  std::string request, response;

  char buff[block_size];

  if (!ctx) {
    ctx = SSL_CTX_new(SSLv23_method());
    if (!ctx) {
      return (std::string());
    }

    bio = BIO_new_ssl_connect(ctx);
    if (!bio) {
      goto fail;
    }

    SSL *ssl = nullptr;
    BIO_get_ssl(bio, &ssl);
    if (!ssl) {
      goto fail;
    }
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    struct hostent *remote_host = gethostbyname(hostname.c_str());
    if (!remote_host) {
      goto fail;
    }

    BIO_ADDR *bio_addr = BIO_ADDR_new();
    if (!bio_addr) {
      BIO_ADDR_free(bio_addr);
      goto fail;
    }

    if (BIO_ADDR_rawmake(bio_addr, AF_INET, remote_host->h_addr,
                         remote_host->h_length, htons(port)) != 1) {
      BIO_ADDR_free(bio_addr);
      goto fail;
    }

    if (BIO_set_conn_address(bio, bio_addr) != 1) {
      BIO_ADDR_free(bio_addr);
      goto fail;
    }

    BIO_ADDR_free(bio_addr);

    if (SSL_get_verify_result(ssl) != X509_V_OK) {
      goto fail;
    }
  } else {
    goto fail;
  }

  ss << "GET ";
  if (!path.empty()) {
    ss << path;
  } else {
    ss << '/';
  }
  ss << " HTTP/1.0\r\nHost: " << hostname << "\r\n";
  if (headers.size()) {
    for (const auto &h : headers) {
      ss << h << "\r\n";
    }
  }
  ss << "User-Agent: HTTP-Client/1.0\r\n"
     << "Connection: close\r\n\r\n";

  request = ss.str();

  BIO_puts(bio, request.c_str());

  int bytes_recv;
  while ((bytes_recv = BIO_read(bio, buff, block_size - 1))) {
    if (bytes_recv == -1) {
      return (std::string());
    }
    buff[bytes_recv] = '\0';
    response.append(buff);
  }

  if (BIO_reset(bio) != 0) {
    return (std::string());
  }

  return response.substr(response.find("\r\n\r\n") + 4);

fail:
  BIO_free_all(bio);
  bio = nullptr;
  SSL_CTX_free(ctx);
  ctx = nullptr;
  return (std::string());
}

std::string Sun::rise() { return cache.rise; }

std::string Sun::set() { return cache.set; }
