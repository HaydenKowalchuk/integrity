#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>

void crc32(uint32_t* crc, const uint8_t* data, size_t len);

static inline uint32_t crc32_string(const char* str) {
  uint32_t crc = 0;
  crc32(&crc, (const uint8_t*)str, strlen(str));
  return crc;
}

static inline uint32_t crc32_data(const void* data, size_t size) {
  uint32_t crc = 0;
  crc32(&crc, (const uint8_t*)data, size);
  return crc;
}
