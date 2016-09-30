#include "jigsaw.h"

// ------------------------------------------------------------------------------------------------ //
//  Defines
// ------------------------------------------------------------------------------------------------ //
#define   LOG_DEBUG(...) {if (current_log_level & JIGSAW_LOG_LEVEL_DEBUG)   {app_log(APP_LOG_LEVEL_DEBUG,   __FILE__, __LINE__, __VA_ARGS__);}}
#define LOG_WARNING(...) {if (current_log_level & JIGSAW_LOG_LEVEL_WARNING) {app_log(APP_LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__);}}
#define   LOG_ERROR(...) {if (current_log_level & JIGSAW_LOG_LEVEL_ERROR)   {app_log(APP_LOG_LEVEL_ERROR,   __FILE__, __LINE__, __VA_ARGS__);}}

// ------------------------------------------------------------------------------------------------ //
//  Internal Variables
// ------------------------------------------------------------------------------------------------ //

static uint8_t current_log_level = JIGSAW_LOG_LEVEL_VERBOSE;  // JIGSAW_LOG_LEVEL_NORMAL // Default log level

static uint8_t *jigsaw = NULL;
static uint32_t jigsaw_size = 0;
static uint32_t jigsaw_bytes_downloaded = 0;
static JigsawStatus status = JIGSAW_STATUS_IDLE;

static void (*jigsaw_download_finished_callback)(uint32_t, uint8_t*) = NULL;  // Variable that holds the callback

// ------------------------------------------------------------------------------------------------ //
// External Functions
// ------------------------------------------------------------------------------------------------ //

JigsawStatus jigsaw_read_iterator(DictionaryIterator *iter) {
  Tuple *jigsaw_init_tuple, *jigsaw_piece_tuple, *jigsaw_index_tuple;

  // Check if we didn't receive a jigsaw piece
  if (!(jigsaw_piece_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_PIECE)))
    return status;  // Didn't receive a jigsaw piece (don't change status)

  // We got a piece!  Get the piece's info
  uint16_t piece_size = jigsaw_piece_tuple->length;
  uint8_t *piece = &jigsaw_piece_tuple->value->uint8;
  uint32_t piece_index = 0;  // Assume 0 for now (index 0 is the first piece when starting new jigsaw)

  // If phone is sending a subsequent piece (most common so checking for that first)
  if ((jigsaw_index_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_PIECE_INDEX))) {
    piece_index = jigsaw_index_tuple->value->uint32;
  // Else, if phone is starting to send new jigsaw
  } else if ((jigsaw_init_tuple = dict_find(iter, MESSAGE_KEY_JIGSAW_INIT))) {
    // Init tuple found: (re)initialize jigsaw
    //if (jigsaw)
    free(jigsaw);

    jigsaw_bytes_downloaded = 0;

    if ((jigsaw = malloc(jigsaw_size = jigsaw_init_tuple->value->uint32))) {
      LOG_DEBUG("Jigsaw: Successfully allocated %d bytes for new jigsaw", (int)jigsaw_size);
    } else {
      LOG_ERROR("Jigsaw: Unable to allocate %d bytes for new jigsaw", (int)jigsaw_size);
      jigsaw_size = 0;
      return (status = JIGSAW_STATUS_FAILED);
    }
  } else { // Else, I don't know what's going on
    LOG_WARNING("Jigsaw: Invalid Jigsaw Piece (no index or init)");
    return status;
  }

  // piece is valid, add it to the jigsaw
  LOG_DEBUG("Jigsaw: Received %d byte piece [%d to %d] of %d",
            (int)piece_size, (int)piece_index, (int)(piece_index + piece_size), (int)jigsaw_size);

  if (!jigsaw) {
    LOG_ERROR("Jigsaw: Jigsaw doesn't exist! Discarding piece.");
    return (status = JIGSAW_STATUS_FAILED);
  }

  // Deep copy the piece into the proper spot (index) in the jigsaw
  memcpy(jigsaw + piece_index, piece, piece_size);

  jigsaw_bytes_downloaded += piece_size;

  // If this is the end piece, then it's fully transferred (since pieces are sent in order)
  if(piece_index + piece_size >= jigsaw_size) {
    LOG_DEBUG("Jigsaw: Transfer Complete");
    // If a "completed" callback was registered, call it now
    if (jigsaw_download_finished_callback)
      (*jigsaw_download_finished_callback)(jigsaw_size, jigsaw);
    return (status = JIGSAW_STATUS_COMPLETE);
  }

  return (status = JIGSAW_STATUS_TRANSFERRING);
}

// ------------------------------------------------------------------------------------------------ //

void jigsaw_destroy() {
  //if (jigsaw)
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
void     jigsaw_set_log_level(uint8_t log_level) {
  current_log_level = log_level;
}
// ------------------------------------------------------------------------------------------------ //
uint8_t  jigsaw_get_log_level() {
  return current_log_level;
}
// ------------------------------------------------------------------------------------------------ //