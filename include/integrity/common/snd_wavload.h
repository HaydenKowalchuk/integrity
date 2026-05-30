#pragma once

#include <integrity/common/common.h>
#include <integrity/common/sound_system.h>

#if SOUND
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int LoadWAVFile(const char* filename, int* format, void** data, int* size, int* freq);
#endif
