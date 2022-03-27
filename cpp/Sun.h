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

class Sun {
public:
  Sun() = delete;

  Sun(float latitude, float longitude);

  std::string rise();
  std::string set();

private:
  BIO *bio = nullptr;
  SSL_CTX *ctx = nullptr;

  struct {
    float latitude;
    float longitude;
    time_t expires;
    char rise[6];
    char set[6];
  } cache;

  std::string https_get(std::string url, std::vector<std::string> headers = {},
                        short port = 443, size_t block_size = 4096);

  std::string utc_to_local(const std::string &utc);
};

#endif
