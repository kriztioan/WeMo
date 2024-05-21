/**
 *  @file   Settings.h
 *  @brief  Settings Class Implementation
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2022-12-24
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include "Settings.h"

Settings::Settings(const std::string &filename) : filename(filename) {

  if (0 > (this->fd_inotify = inotify_init())) {

    Log::perror("Failed to initialize inotify");
    return;
  }

  if (-1 ==
      (this->wd_inotify = inotify_add_watch(this->fd_inotify, filename.c_str(),
                                            IN_CLOSE_WRITE | IN_IGNORED))) {

    Log::perror("Failed to add %s watch", filename.c_str());
    return;
  }

  if (md5sum() == -1) {

    Log::err("Failed to compute MD5");
    return;
  }

  parse();
}

Settings::~Settings() {

  if (0 < this->fd_inotify) {

    inotify_rm_watch(this->fd_inotify, this->wd_inotify);

    close(this->fd_inotify);
  }
}

int Settings::parse() {

  ini.clear();

  size_t n = 64;

  char *l = (char *)malloc(n), s[64], k[64], v[64];

  FILE *f = NULL;
  if (NULL == (f = fopen(this->filename.c_str(), "r"))) {

    Log::perror("Failed to open ini-file", this->filename.c_str());

    return errno;
  }

  while (getline(&l, &n, f) != -1) {

    if (*l == '\n' || *l == '#' || *l == ';') {

      continue;
    }

    if (sscanf(l, "[%[^]]", s) == 1) {

      continue;
    }

    if (sscanf(l, "%[^=]=%s ", k, v) == 2) {

      ini[s][k] = v;
    }
  }

  free(l);

  fclose(f);

  return 0;
}

int Settings::handler() {

  ssize_t size = 128 * sizeof(struct inotify_event), i = 0;

  char buff[size];

  ssize_t len;

  if ((len = read(this->fd_inotify, buff, size)) < 0) {

    Log::perror("Failed to read inotify file descriptor");

    return errno;
  }

  bool triggered = false;

  while (i < len) {

    struct inotify_event *e = (struct inotify_event *)(buff + i);

    if (e->mask & IN_CLOSE_WRITE) {

      triggered = true;
    }

    if (e->mask & IN_IGNORED) {

      if (-1 == (this->wd_inotify =
                     inotify_add_watch(this->fd_inotify, filename.c_str(),
                                       IN_CLOSE_WRITE | IN_IGNORED))) {

        Log::perror("Failed to re-add %s watch", filename.c_str());
        return -1;
      }

      triggered = true;
    }

    i += (sizeof(struct inotify_event) + e->len);
  }

  if (triggered && md5sum() == 1) {

    Log::info("Settings changed");

    return parse();
  }

  return -1;
}

int Settings::md5sum() {

  std::ifstream ifstr(this->filename, std::ios::in);

  if (ifstr.fail()) {

    Log::perror("Failed to open ini-file", this->filename.c_str());

    return errno;
  }

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();

  if (ctx == NULL) {

    Log::err("Failed to create MD5 context");
    return -1;
  }

  if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1) {

    Log::err("Failed initialize MD5");
    return -1;
  }

  char bytes[4096];

  while (size_t len = ifstr.readsome(bytes, 4096)) {

    if (EVP_DigestUpdate(ctx, bytes, len) != 1) {

      EVP_MD_CTX_free(ctx);

      Log::err("Failed to update MD5");
      return -1;
    }
  }

  unsigned char digest[MD5_DIGEST_LENGTH];

  unsigned int digest_size = MD5_DIGEST_LENGTH;

  if (EVP_DigestFinal_ex(ctx, digest, &digest_size) != 1) {

    EVP_MD_CTX_free(ctx);

    Log::err("Failed to finalize MD5");
    return -1;
  }

  EVP_MD_CTX_free(ctx);

  ifstr.close();

  if (memcmp(md5, digest, digest_size) != 0) {

    memcpy(md5, digest, digest_size);

    return 1;
  }

  return 0;
}
