#pragma once
#include <pebble.h>

// Logging Levels (Add together to log multiple levels)
#define JIGSAW_LOG_LEVEL_DEBUG   0b001  // 1: Log unnecessary messages
#define JIGSAW_LOG_LEVEL_WARNING 0b010  // 2: Log warnings
#define JIGSAW_LOG_LEVEL_ERROR   0b100  // 4: Log fatal errors

// Combined Logging Levels:
#define JIGSAW_LOG_LEVEL_NORMAL  0b110  // Standard Logging Level (warnings and errors)
#define JIGSAW_LOG_LEVEL_VERBOSE 255    // Log everything


typedef enum {
  JIGSAW_STATUS_IDLE         = 0x00, // Jigsaw not allocated, no init received
  JIGSAW_STATUS_TRANSFERRING = 0x01, // Init received, jigsaw partially complete
  JIGSAW_STATUS_COMPLETE     = 0x02, // Jigsaw finished, final piece received
  JIGSAW_STATUS_FAILED       = 0x03, // Jigsaw failed to allocate memory
} JigsawStatus;

JigsawStatus jigsaw_read_iterator(DictionaryIterator *iter);
JigsawStatus jigsaw_get_status();

void jigsaw_subscribe(void (*download_finished_callback)(uint32_t, uint8_t*));
void jigsaw_unsubscribe();
void jigsaw_destroy();

uint32_t jigsaw_get_size();
uint8_t *jigsaw_get_data();
uint32_t jigsaw_get_bytes_downloaded();
uint8_t  jigsaw_get_percent_downloaded();
bool     jigsaw_completed();
uint8_t  jigsaw_get_log_level();
void     jigsaw_set_log_level(uint8_t log_level);
