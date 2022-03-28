/**
 *  @file   Sun.cpp
 *  @brief  Sun Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-03-25
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Sun.h"

const char *Sun::store_file = "sun.store";

Sun::Sun(float latitude, float longitude)
    : latitude(latitude), longitude(longitude) {

  if (read_store() == 0 && validate_store() == 0) {

    return;
  }

  store.latitude = latitude;

  store.longitude = longitude;

  time_t t = time(NULL);

  char s[11];
  strftime(s, 11, "%F", localtime(&t));

  std::stringstream url;
  url << "api.sunrise-sunset.org/json?lat=" << latitude << "&lng=" << longitude
      << "&date=" << s << "&formatted=0";

  std::vector<std::string> headers = {"Accept: application/json"};

  std::string json = https_get(url.str(), headers);

  rapidjson::Document rapid;
  if (!rapid.Parse(json.c_str()).HasParseError()) {
    if (strncmp(rapid["status"].GetString(), "OK", 2) == 0) {

      strncpy(store.rise,
              utc_to_local(rapid["results"]["sunrise"].GetString()).c_str(),
              17);
      strncpy(store.set,
              utc_to_local(rapid["results"]["sunset"].GetString()).c_str(), 17);
    }
  }

  write_store();
}

int Sun::read_store() {

  int fd = open(store_file, O_RDONLY);
  if (fd > 0) {
    read(fd, &store, sizeof(store));
    close(fd);
    return 0;
  }

  return -1;
}

int Sun::validate_store() {

  time_t t = time(NULL);

  char s[11];
  if (strftime(s, 11, "%F", localtime(&t)) && store.latitude == latitude &&
      store.longitude == longitude && strncmp(store.rise, s, 10) == 0) {

    return 0;
  }

  return -1;
}

void Sun::write_store() {

  int fd = open(store_file, O_WRONLY | O_CREAT, 0644);
  if (fd > 0) {
    write(fd, &store, sizeof(store));
    close(fd);
  }
}

std::string Sun::utc_to_local(const std::string &utc) {

  struct tm s_tm;

  char *err;
  if ((err = strptime(utc.c_str(), "%FT%T%z", &s_tm)) != NULL && !*err) {

    time_t t = timegm(&s_tm);
    if (localtime_r(&t, &s_tm)) {

      char s[17];
      if (strftime(s, 17, "%FT%-k:%M", &s_tm)) {

        return s;
      }
    }
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

std::string Sun::rise() { return store.rise + 11; }

std::string Sun::set() { return store.set + 11; }
