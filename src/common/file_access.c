#include <ctype.h>
#include <dirent.h>
#include <integrity/common/file_access.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define GAMEJAM_LOG_LEVEL 0
#define GAMEJAM_LOG_GROUP "FS"
#include <gamejam/log.h>

void FS_DebugDumpFolder(const char *path);

// Initialize with the current directory as the first search path
static char fs_search_paths[FS_MAX_PATHS][FS_MAX_PATH] = {""};
static int fs_num_paths = 1;
static char scratch_path[FS_MAX_PATH] = {0};

void FS_ClearSearchPaths(void) {
  fs_num_paths = 0;
}

/**
 * Adds a base directory to the search chain.
 */
void FS_AddSearchPath(const char *path) {
  GAMEJAM_LOG_DEBUG("%s called!", __func__);

  if (fs_num_paths >= FS_MAX_PATHS) {
    GAMEJAM_LOG_ERROR("%s: too many paths!", __func__);
    return;
  }

  if (!FS_DirExists(path)) {
    GAMEJAM_LOG_ERROR("%s: Cannot open folder %s", __func__, path);
    return;
  }

  FS_DebugDumpFolder(path);

  strncpy(fs_search_paths[fs_num_paths], path, FS_MAX_PATH - 1);

  // Ensure the path ends with a slash for easy concatenation
  size_t len = strlen(fs_search_paths[fs_num_paths]);
  if (len > 0 && fs_search_paths[fs_num_paths][len - 1] != '/' &&
      len < FS_MAX_PATH - 1) {
    strcat(fs_search_paths[fs_num_paths], "/");
  }

  fs_num_paths++;
}

void FS_DebugDumpFolder(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    GAMEJAM_LOG_ERROR("DEBUG: Cannot open folder %s", path);
    return;
  }

  struct dirent *entry;
  GAMEJAM_LOG_INFO("--- REAL HARDWARE DIRECTORY DUMP (%s) ---", path);
  while ((entry = readdir(dir)) != NULL) {
    GAMEJAM_LOG_INFO(" Found file: '%s'", entry->d_name);
  }
  GAMEJAM_LOG_INFO("----------------------------------------");
  closedir(dir);
}

static void FS_TargetSubpathToUpper(char *path, const char *search_prefix) {
  size_t prefix_len = strlen(search_prefix);

  // Safety check: ensure the path is at least as long as the prefix
  if (strlen(path) < prefix_len) {
    return;
  }

  // Advance the pointer past the search prefix to only modify the user's subpath
  char *subpath = path + prefix_len;

  // Uppercase everything that follows (subfolders + filename)
  for (int i = 0; subpath[i]; i++) {
    subpath[i] = (char)toupper((unsigned char)subpath[i]);
  }
}

/**
 * ISO9660 fix: replaces the first dot with an underscore if multiple dots
 * exist.
 */
// static void FS_NormalizeISO(char *path) {
//   int first_dot = -1;
//   int dot_count = 0;

//   for (int i = 0; path[i]; i++) {
//     if (path[i] == '.') {
//       dot_count++;
//       if (first_dot == -1) {
//         first_dot = i;
//       }
//     }
//   }

//   if (dot_count > 1 && first_dot != -1) {
//     path[first_dot] = '_';
//   }
// }

/**
 * Searches the chain for 'filename'.
 * If found, fills 'out_path' and returns true.
 */
bool FS_ResolvePath(const char *filename, char *out_path) {
  GAMEJAM_LOG_DEBUG("[FS] Searching for %s", filename);

  if (filename && filename[0] == '/') {
    for (int i = 0; i < fs_num_paths; i++) {
      if (strncmp(filename, fs_search_paths[i], strlen(fs_search_paths[i])) == 0) {
        struct stat st;
        if (stat(filename, &st) == 0) {
          memset(out_path, 0, FS_MAX_PATH);
          strncpy(out_path, filename, FS_MAX_PATH - 1);
          GAMEJAM_LOG_DEBUG("[FS] Using prefixed: %s", out_path);
          return true;
        }
      }
    }
  }

  char buffer[FS_MAX_PATH];
  for (int i = 0; i < fs_num_paths; i++) {
    memset(buffer, 0, FS_MAX_PATH);

    // Combine search path + filename
    snprintf(buffer, FS_MAX_PATH, "%s%s", fs_search_paths[i], filename);
    GAMEJAM_LOG_DEBUG("[FS] Checking %s", buffer);

    // Apply ISO9660 constraints
    // FS_NormalizeISO(buffer);

    // Try original case first (Works on PC / Emulators / Fixed ISOs)
    if (FS_FileExists(buffer)) {
      memset(out_path, 0, FS_MAX_PATH);
      strncpy(out_path, buffer, FS_MAX_PATH - 1);
      GAMEJAM_LOG_DEBUG("[FS] Found %s", out_path);
      return true;
    }

    //  Fallback: Convert the entire path string to uppercase for real PSP hardware
    // e.g. "disc0:/PSP_GAME/USRDIR/model.obj" -> "DISC0:/PSP_GAME/USRDIR/MODEL.OBJ"
    FS_TargetSubpathToUpper(buffer, fs_search_paths[i]);
    GAMEJAM_LOG_DEBUG("[FS] Checking Uppercase Fallback: %s", buffer);

    if (FS_FileExists(buffer)) {
      memset(out_path, 0, FS_MAX_PATH);
      strncpy(out_path, buffer, FS_MAX_PATH - 1);
      GAMEJAM_LOG_DEBUG("[FS] Found Uppercase: %s", out_path);
      return true;
    }
  }

  GAMEJAM_LOG_DEBUG("[FS] not found!");
  return false;
}

/**
 * Searches the chain for 'filename'.
 * If found, returns path, otherwise null
 */
const char *FS_ResolvePathTemp(const char *filename) {
  return FS_ResolvePath(filename, scratch_path) ? scratch_path : NULL;
}

/**
 * Original utility moved to FS_ prefix
 */
int FS_FileLength(FILE *f) {
  int pos = ftell(f);
  fseek(f, 0, SEEK_END);
  int end = ftell(f);
  fseek(f, pos, SEEK_SET);
  return end;
}

/**
 * Takes a resolved path
 */
// bool FS_FileExists(const char *path) {
//   struct stat buffer;
//   return (stat(path, &buffer) == 0);
// }
/**
 * Takes a resolved path and verifies existence via handle opening
 */
bool FS_FileExists(const char *path) {
  // Open the file directly. If the PSP kernel can't resolve the extent,
  // this returns NULL immediately, which is incredibly accurate.
  FILE *f = fopen(path, "rb");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

bool FS_DirExists(const char *path) {
  DIR *dir = opendir(path);
  if (!dir) {
    GAMEJAM_LOG_DEBUG("[FS] dir not found %s!", path);
    return false;
  }

  closedir(dir);
  return true;
}
