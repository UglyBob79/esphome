#pragma once
#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "esphome/components/display/display_buffer.h"
#include "esphome/core/log.h"

static const char *EINK_TAG = "eink_fetch";

// 480 * 800 / 8 = 48000 bytes
// BSS allocation avoids the 32 KB max-contiguous heap limit on ESP32.
static uint8_t EINK_IMAGE_BUF[48000];
static bool    EINK_IMAGE_READY = false;

// Access the protected buffer_ member of DisplayBuffer.
// Never instantiated — only provides static cast access.
struct DisplayBufAccessor : public esphome::display::DisplayBuffer {
  static uint8_t *get(esphome::display::Display *d) {
    auto *db = static_cast<esphome::display::DisplayBuffer *>(d);
    return static_cast<DisplayBufAccessor *>(db)->buffer_;
  }
};

static bool eink_fetch(const char *host, uint16_t port, const char *path) {
  struct hostent *he = ::gethostbyname(host);
  if (!he) {
    ESP_LOGE(EINK_TAG, "DNS lookup failed for %s", host);
    return false;
  }

  int sock = ::socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    ESP_LOGE(EINK_TAG, "socket() failed");
    return false;
  }

  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  memcpy(&addr.sin_addr, he->h_addr, he->h_length);

  if (::connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
    ESP_LOGE(EINK_TAG, "connect() failed");
    ::close(sock);
    return false;
  }

  // Send minimal HTTP/1.0 request (no chunked encoding, connection closes after body)
  char req[256];
  snprintf(req, sizeof(req),
           "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
           path, host);
  ::send(sock, req, strlen(req), 0);

  // Skip response headers — read until blank line
  char c, prev = 0;
  int nl = 0;
  uint32_t t = millis();
  while (millis() - t < 5000) {
    if (::recv(sock, &c, 1, MSG_DONTWAIT) == 1) {
      if (c == '\n') {
        if (nl == 1) break;  // \r\n\r\n → blank line reached
        nl = (prev == '\r') ? 1 : 0;
      } else if (c != '\r') {
        nl = 0;
      }
      prev = c;
    }
    App.feed_wdt();
  }

  // Stream body into static buffer
  int offset = 0;
  uint32_t deadline = millis() + 10000;
  while (offset < 48000 && millis() < deadline) {
    int n = ::recv(sock, EINK_IMAGE_BUF + offset, 48000 - offset, 0);
    if (n > 0) offset += n;
    else if (n == 0) break;  // connection closed
    App.feed_wdt();
  }
  ::close(sock);

  if (offset < 48000) {
    ESP_LOGE(EINK_TAG, "Short read: %d / 48000 bytes", offset);
    return false;
  }
  EINK_IMAGE_READY = true;
  ESP_LOGI(EINK_TAG, "Fetched 48000 bytes OK");
  return true;
}
