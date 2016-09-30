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




/*
Note: The same piece could transfer twice (replacing old piece) adding to the total bytes downloaded but wouldn't help complete the jigsaw any further.

How the Javascript sends pieces:
  {"JIGSAW_INIT": data.length, "JIGSAW_PIECE":piece} // First Piece
  {"JIGSAW_PIECE_INDEX":bytes, "JIGSAW_PIECE":piece} // Subsequent Pieces

*/

/*
// typedef enum {
//   JIGSAW_REASON_NO_ERROR = 0x00;
//   JIGSAW_REASON_OUT_OF_MEMORY = 0x14,  // failed to allocate memory
//   JIGSAW_REASON_NOT_ALLOCATED = 0x15,  // trying to add piece without allocation
//   JIGSAW_REASON_INVALID_PIECE = 0x26, //Invalid Jigsaw Piece
// } JigsawReason;


// Return results
//  0x0X: Normal Stauts
//  0x1X: Fatal Error
//  0x2X: Warning
typedef enum {
  JIGSAW_STATUS_IDLE         = 0x00,
  JIGSAW_STATUS_TRANSFERRING = 0x01,
  JIGSAW_STATUS_COMPLETE     = 0x02,
  JIGSAW_STATUS_FAILED       = 0x03,
  
  JIGSAW_STATUS_ERROR_OUT_OF_MEMORY = 0x13,  // failed to allocate memory
  JIGSAW_STATUS_ERROR_NOT_ALLOCATED = 0x13,  // trying to add piece without allocation
  JIGSAW_STATUS_WARNING_INVALID_PIECE = 0x24, //Invalid Jigsaw Piece
} JigsawStatus;


#include "jigsaw.h"

// ------------------------------------------------------------------------------------------------ //
//  Defines
// ------------------------------------------------------------------------------------------------ //
#define   LOG_DEBUG(...) {if (log_level & JIGSAW_LOG_LEVEL_DEBUG)   {app_log(APP_LOG_LEVEL_DEBUG,   __FILE__, __LINE__, __VA_ARGS__);}}
#define LOG_WARNING(...) {if (log_level & JIGSAW_LOG_LEVEL_WARNING) {app_log(APP_LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__);}}
#define   LOG_ERROR(...) {if (log_level & JIGSAW_LOG_LEVEL_ERROR)   {app_log(APP_LOG_LEVEL_ERROR,   __FILE__, __LINE__, __VA_ARGS__);}}

// ------------------------------------------------------------------------------------------------ //
//  Internal Variables
// ------------------------------------------------------------------------------------------------ //

static uint8_t log_level = JIGSAW_LOG_LEVEL_VERBOSE;  // JIGSAW_LOG_LEVEL_NORMAL // Default log level

static uint8_t *jigsaw = NULL;
static uint32_t jigsaw_size = 0;
static uint32_t jigsaw_bytes_downloaded = 0;
static JigsawStatus status = JIGSAW_STATUS_IDLE;

static void (*jigsaw_download_finished_callback)(uint32_t, uint8_t*) = NULL;  // Variable that holds the callback


// ------------------------------------------------------------------------------------------------ //
// Internal Functions
// ------------------------------------------------------------------------------------------------ //
// (re)initializes jigsaw
static void init_jigsaw(uint32_t total_size) {
  if (jigsaw)
    free(jigsaw);
  
  jigsaw_bytes_downloaded = 0;
  
  if ((jigsaw = malloc(jigsaw_size = total_size))) {
    LOG_DEBUG("Jigsaw: Successfully allocated %d bytes for new jigsaw", (int)jigsaw_size);
  } else {
    LOG_ERROR("Jigsaw: Unable to allocate %d bytes for new jigsaw", (int)jigsaw_size);
    jigsaw_size = 0;
  }
}


// ------------------------------------------------------------------------------------------------ //


// Deep copy the piece into the proper spot (index) in the jigsaw
static JigsawStatus jigsaw_add_piece(uint8_t *piece, uint32_t piece_index, uint16_t piece_size) {
  LOG_DEBUG("Jigsaw: Received %d byte piece [%d to %d] of %d",
        (int)piece_size, (int)piece_index, (int)(piece_index + piece_size), (int)jigsaw_size);

  if (!jigsaw) {
    LOG_ERROR("Jigsaw: Jigsaw doesn't exist! Discarding piece.");
    return JIGSAW_STATUS_ERROR_NOT_ALLOCATED;
  }

  memcpy(jigsaw + piece_index, piece, piece_size);
  
  jigsaw_bytes_downloaded += piece_size;
  
  // If this is the end piece, then it's fully transferred (since pieces are sent in order)
  if(piece_index + piece_size >= jigsaw_size) {
    LOG_DEBUG("Jigsaw: Transfer Complete");
    // If a "completed" callback was registered, call it now
    if (jigsaw_download_finished_callback)
      (*jigsaw_download_finished_callback)(jigsaw_size, jigsaw);
    return JIGSAW_STATUS_COMPLETE;
  }

  return JIGSAW_STATUS_TRANSFERRING;
}


// ------------------------------------------------------------------------------------------------ //
// External Functions
// ------------------------------------------------------------------------------------------------ //

JigsawStatus jigsaw_read_iterator(DictionaryIterator *iter) {
  Tuple *jigsaw_init_tuple, *jigsaw_piece_tuple, *jigsaw_index_tuple;

  // Check if we didn't receive a jigsaw piece
  if (!(jigsaw_piece_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_PIECE)))
    return JIGSAW_STATUS_IDLE;  // Didn't receive a jigsaw piece (don't change status)

  // Get the piece's info
  uint16_t piece_size = jigsaw_piece_tuple->length;
  uint8_t *piece = &jigsaw_piece_tuple->value->uint8;
  uint32_t piece_index = 0;  // Assume 0 for now (index 0 is the first piece when starting new jigsaw)

  // If phone is sending a subsequent piece (most common so checking for that first)
  if ((jigsaw_index_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_PIECE_INDEX))) {
    piece_index = jigsaw_index_tuple->value->uint32;
    // Else, if phone is starting to send new jigsaw
  } else if ((jigsaw_init_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_INIT))) {
    init_jigsaw(jigsaw_init_tuple->value->uint32);
  } else { // Else, I don't know what's going on
    LOG_WARNING("Jigsaw: Invalid Jigsaw Piece (no index or init)");
    return JIGSAW_STATUS_WARNING_INVALID_PIECE;
  }

  // piece is valid, add it to the jigsaw
  return (status = jigsaw_add_piece(piece, piece_index, piece_size));
}


// ------------------------------------------------------------------------------------------------ //

void jigsaw_destroy() {
  if (jigsaw)
    free(jigsaw);
  jigsaw = NULL;
  jigsaw_size = 0;
  jigsaw_bytes_downloaded = 0;
  status = JIGSAW_STATUS_IDLE;
}

// ------------------------------------------------------------------------------------------------ //
void jigsaw_subscribe(void (*download_finished_callback)(uint32_t, uint8_t*)) {
  jigsaw_download_finished_callback = download_finished_callback;
}
// ------------------------------------------------------------------------------------------------ //
void jigsaw_unsubscribe() {
  jigsaw_download_finished_callback = NULL;
}
// ------------------------------------------------------------------------------------------------ //
uint32_t jigsaw_get_size() {
  return jigsaw_size;
}
// ------------------------------------------------------------------------------------------------ //
uint8_t *jigsaw_get_data() {
  return jigsaw;
}
// ------------------------------------------------------------------------------------------------ //
uint32_t jigsaw_get_bytes_downloaded() {
  return jigsaw_bytes_downloaded;
}
// ------------------------------------------------------------------------------------------------ //
uint8_t  jigsaw_get_percent_downloaded() {
  return jigsaw_size ? (jigsaw_bytes_downloaded * 100 / jigsaw_size) : 0;
}
// ------------------------------------------------------------------------------------------------ //
bool     jigsaw_completed() {
  //return (jigsaw_status == JIGSAW_STATUS_COMPLETE); // Should work, too
  return ((jigsaw_size > 0) && jigsaw_bytes_downloaded >=jigsaw_size);
}
// ------------------------------------------------------------------------------------------------ //
JigsawStatus jigsaw_get_status() {
  return status;
}
// ------------------------------------------------------------------------------------------------ //
void     jigsaw_set_debug_level(uint8_t debug_level) {
  log_level = debug_level;
}
// ------------------------------------------------------------------------------------------------ //
uint8_t  jigsaw_get_debug_level() {
  return log_level;
}
// ------------------------------------------------------------------------------------------------ //
*/