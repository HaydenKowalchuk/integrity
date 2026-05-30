#pragma once

#include <integrity/common/common.h>

typedef void (*destroy_func)(void*);

typedef struct resource_type {
  destroy_func destroy;
  char type;
} resource_type;

typedef struct resource_object {
  void* pointer;
  uint32_t crc;
  int count;
  char type;
} resource_object;

void resource_test(void);

/* Object management */
void resource_object_add(char identifier, uint32_t crc, void* pointer);
// bool resource_object_delete(uint32_t crc);
void resource_object_remove(uint32_t crc);
int resource_object_find(uint32_t crc);
int resource_object_count(uint32_t crc);
void* resource_object_pointer(uint32_t crc);

void resource_objects_empty(void);

/* Clear all with count <= 0 */
void resource_objects_flush(void);

/* Dump Stats about currently known data */
void resource_add_handler(char identifier, destroy_func destroy);
void resource_print_handlers(void);
void resource_print_objects(void);

void crc32(uint32_t* crc, const uint8_t* data, size_t len);
