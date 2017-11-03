#pragma once
#include <pebble.h>

// Logging Levels
// Note: Add together to log multiple levels
//       e.g.  jigsaw_set_log_level(JIGSAW_LOG_LEVEL_DEBUG + JIGSAW_LOG_LEVEL_ERROR)
#define JIGSAW_LOG_LEVEL_DEBUG   0b001  // 1: Log unnecessary messages
#define JIGSAW_LOG_LEVEL_WARNING 0b010  // 2: Log warnings
#define JIGSAW_LOG_LEVEL_ERROR   0b100  // 4: Log fatal errors

// Combined Logging Levels:
#define JIGSAW_LOG_LEVEL_NORMAL  0b110  // Standard Logging Level (warnings and errors)
#define JIGSAW_LOG_LEVEL_VERBOSE 255    // Log everything

// Statuses returned by jigsaw_read_iterator() and jigsaw_get_status()
typedef enum {
  JIGSAW_STATUS_IDLE         = 0x00, // Jigsaw not allocated, no init received
  JIGSAW_STATUS_TRANSFERRING = 0x01, // Init received, jigsaw partially complete
  JIGSAW_STATUS_COMPLETE     = 0x02, // Jigsaw finished, final piece received
  JIGSAW_STATUS_FAILED       = 0x03, // Jigsaw failed to allocate memory
} JigsawStatus;

JigsawStatus jigsaw_get_status();
JigsawStatus jigsaw_read_iterator(DictionaryIterator *iter);
//Put jigsaw_read_iterator() into your app message receive handler.
//  Example:
//  app_message_register_inbox_received(appmessage_in_received_handler);
//  static void appmessage_in_received_handler(DictionaryIterator *iter, void *context) {
//    jigsaw_read_iterator(iter);
//  }



void jigsaw_subscribe(void (*download_finished_callback)(uint32_t, uint8_t*));
//To subscribe:
//  1) Make a callback function in the form of:
//    static void download_finished_callback(uint32_t data_size, uint8_t *data) {}
//  2) Then subscribe by passing the function name:
//    jigsaw_subscribe(download_finished_callback);
//To point the download-finished callback to a new function, just call jigsaw_subscribe() again.
//To stop the download-finished callback from being called, call jigsaw_unsubscribe();
void jigsaw_unsubscribe();
//After you are done with the downloaded data, you can free the memory by calling:
void jigsaw_destroy();
//Note that you don't need to call jigsaw_destroy() between downloads as the
//  memory is freed and malloc'd after each INIT received from the JS side.


uint32_t jigsaw_get_size();
uint8_t *jigsaw_get_data();
uint32_t jigsaw_get_bytes_downloaded();
uint8_t  jigsaw_get_percent_downloaded();
bool     jigsaw_completed();  // Is completed?
uint8_t  jigsaw_get_log_level();
void     jigsaw_set_log_level(uint8_t log_level);
