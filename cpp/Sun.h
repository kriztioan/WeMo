/**
 *  @file   Sun.h
 *  @brief  Sun Class Definition
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-03-25
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#ifndef SUN_H_
#define SUN_H_

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/opensslconf.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include "Log.h"

class Sun {
public:
  Sun() = delete;

  Sun(float latitude, float longitude);

  std::string rise();
  std::string set();

private:
  BIO *bio = nullptr;
  SSL_CTX *ctx = nullptr;

  float latitude;
  float longitude;

  struct {
    float latitude;
    float longitude;
    char rise[17];
    char set[17];
  } store = {};

  static const char *store_file;

  int read_store();
  int validate_store();
  void write_store();

  std::string https_get(std::string url, std::vector<std::string> headers = {},
                        short port = 443, size_t block_size = 4096);

  std::string utc_to_local(const std::string &utc);
};

#endif
