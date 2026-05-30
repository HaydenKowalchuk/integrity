#include <integrity/common/resource_manager.h>

void crc32(uint32_t *crc, const uint8_t *data, size_t len) {
  *crc = ~*crc;
  for (size_t i = 0; i < len; i++) {
    for (int j = 0; j < 8; j++) {
      uint32_t bit = (*crc ^ (data[i] >> j)) & 1;
      *crc = (*crc >> 1) ^ ((-bit) & UINT32_C(0xEDB88320));
    }
  }
  *crc = ~*crc;
}

#include <integrity/common/image_loader.h>
#include <integrity/common/obj_loader.h>

#define MAX_OBJECTS 64
#define MAX_TYPES 4

static int num_objects = 0;

static resource_type resource_types[MAX_TYPES];
static resource_object objects[MAX_OBJECTS];

/* ALTERNATIVE */
/*
if you want a non-colliding checksum on a 32-bit system
I like to mix a basic summed checksum
the length and the first 4 characters into a single int32_t

ntoskrnl.exe Today at 2:46 PM
it's a single-instruction compare
and then the length can be compared separately
yeah shifted

*/

static void nop_func(void *_) {
  (void)_;
}

static inline destroy_func resource_get_handler(char identifier) {
  int iter = 0;
  do {
    if (resource_types[iter].type == identifier) {
      return resource_types[iter].destroy;
    }
  } while (resource_types[++iter].type != '\0');
  return &nop_func;
}

void resource_add_handler(char identifier, destroy_func destroy) {
  int iter = 0;
  do {
    if (resource_types[iter].type == '\0') {
      resource_types[iter] = (resource_type){.type = identifier, .destroy = destroy};
      break;
    }
  } while (resource_types[iter++].type != '\0');
}

void resource_print_handlers(void) {
  printf("\nResource Handlers Registered\n------------\n");
  int iter = 0;
  do {
    if (resource_types[iter].type != '\0') {
      printf("Handler [type:%c, func:%p]\n", resource_types[iter].type, resource_types[iter].destroy);
    }
  } while (resource_types[++iter].type != '\0');
}

int resource_object_find(uint32_t crc) {
  int iter = 0;
  do {
    if (objects[iter].crc == crc) {
      return iter;
    }
  } while (++iter < num_objects);
  return -1;
}

int resource_object_count(uint32_t crc) {
  int iter = 0;
  do {
    if (objects[iter].crc == crc) {
      return objects[iter].count;
    }

  } while (++iter < num_objects);
  return -1;
}

void *resource_object_pointer(uint32_t crc) {
  int iter = 0;
  do {
    if (objects[iter].crc == crc) {
      return objects[iter].pointer;
    }
  } while (++iter < num_objects);
  assert(0 && "DIDNT DO ERROR CHECKING, CHECK STACK!");
  return NULL;
}

void resource_object_add(char identifier, uint32_t crc, void *pointer) {
  int index = resource_object_find(crc);
  if (index == -1) {
    objects[num_objects++] = (resource_object){.type = identifier, .pointer = pointer, .crc = crc, .count = 1};
  } else {
    /* Already added */
    objects[index].count++;
    // printf(" INCREMENT Object with crc: %08X count now %d\n", (unsigned int)objects[index].crc, objects[index].count);
  }
}

void resource_object_remove(uint32_t crc) {
  int index = resource_object_find(crc);
  if (index == -1) {
    /* @Todo: need error handling better than assert */
    // assert(0 && "Resource to remove doesn't exist!");
  } else {
    /* Already added */
    objects[index].count--;
    // printf("DECREMENT Object with crc: %08X count now %d\n", (unsigned int)objects[index].crc, objects[index].count);
  }
}

bool resource_object_delete(uint32_t crc) {
  int index = resource_object_find(crc);
  if (index == -1) {
    /* @Todo: need error handling better than assert */
    return false;
  } else {
#ifdef DEBUG
    printf("removing object %d\n", index);
#endif

    // There's one fewer.
    num_objects--;

    // Swap it with the last active resource
    // right before the inactive ones.
    resource_object temp = objects[num_objects];
    objects[num_objects] = objects[index];
    objects[index] = temp;

    memset(&objects[num_objects], '\0', sizeof(resource_object));
    return true;
  }
}

void resource_objects_empty(void) {
  int iter = 0;
  do {
    if (objects[iter].type != '\0') {
      if (objects[iter].pointer != NULL) {
        destroy_func _destroy = resource_get_handler(objects[iter].type);
        (*_destroy)(&objects[iter].pointer);
        objects[iter].pointer = NULL;
      }
      memset(&objects[iter], '\0', sizeof(resource_object));
    }
  } while (++iter < num_objects);
}

void resource_objects_flush(void) {
  int iter = 0;
  do {
    if (objects[iter].count <= 0 && (objects[iter].type != '\0')) {
#ifdef DEBUG
      printf("removing object %d\n", iter);
#endif
      if (objects[iter].pointer != NULL) {
        destroy_func _destroy = resource_get_handler(objects[iter].type);
        (*_destroy)(&objects[iter].pointer);
        objects[iter].pointer = NULL;
      }

      // There's one fewer.
      num_objects--;

      // Swap it with the last active resource
      // right before the inactive ones.
      resource_object temp = objects[num_objects];
      objects[num_objects] = objects[iter];
      objects[iter] = temp;

      memset(&objects[num_objects], 0, sizeof(resource_object));

      // Iterate correctly
      iter--;
    }
  } while (++iter <= num_objects);
}

void resource_print_objects(void) {
  printf("\nObjects Registered\n------------\n");
  int iter = 0;
  do {
    if (objects[iter].type != '\0') {
      printf("Object[%02d] [type:%c ptr:%p, crc:%08X, count:%d]\n", iter, objects[iter].type, objects[iter].pointer, (unsigned int)objects[iter].crc, objects[iter].count);
      if (objects[iter].type == 'i')
        printf("\tname: %s\n", ((tx_image *)(objects[iter].pointer))->name);
    }
  } while (++iter < num_objects);
}

#if 0
/* Testing */
static void INT_destroy(void *pointer)
{
    printf("Destroyed and Freed Int\n");
    (void)pointer;
}

static void STR_destroy(void *pointer)
{
    printf("Destroyed and Freed String\n");
    (void)pointer;
}
void resource_test(void);

int main(int argc, char **argv)
{
    resource_test();
}
#endif

void resource_test(void) {
  /* Reset all stored stuff */
  memset(resource_types, '\0', sizeof(resource_types));
  memset(objects, '\0', sizeof(objects));

  /* Add basic handlers */
  resource_add_handler('i', (destroy_func)&IMG_destroy);
  resource_add_handler('o', (destroy_func)&OBJ_destroy);

#ifdef DEBUG
  resource_print_handlers();
#endif

#if 0
    /* Add data to track */
    int data1 = 56;
    int data2 = 121;

    resource_object_add('i', 1, &data1);
    resource_object_add('i', 2, &data2);
    resource_object_add('i', 2, &data2);
    resource_object_add('i', 3, &data2);
    resource_object_add('i', 4, &data2);
    resource_object_add('i', 5, &data2);

    resource_print_objects();

    resource_object_remove(3);

    resource_print_objects();

    resource_objects_flush();
    resource_print_objects();

#endif
}
