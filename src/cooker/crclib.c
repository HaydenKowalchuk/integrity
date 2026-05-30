#include <cooker/crclib.h>
#include <string.h>

void crc32(uint32_t* crc, const uint8_t* data, size_t len) {
  *crc = ~*crc;
  for (size_t i = 0; i < len; i++) {
    for (int j = 0; j < 8; j++) {
      uint32_t bit = (*crc ^ (data[i] >> j)) & 1;
      *crc = (*crc >> 1) ^ ((-bit) & UINT32_C(0xEDB88320));
    }
  }
  *crc = ~*crc;
}
