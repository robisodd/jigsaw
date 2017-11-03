var MessageQueue   = require('message-queue-pebble');
//message-queue-pebble found at: https://www.npmjs.com/package/message-queue-pebble

exports.debug = true;
exports.abort = false;
exports.bytes_sent = 0;
exports.size = 0;
exports.piece_size = 0;
exports.piece = null;
exports.piece_max_size = 1000;  // Default piece size

module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  function send_next_piece() {
    exports.bytes_sent += exports.piece_size;
    
    if (piece_success_callback) piece_success_callback();  // Call piece success callback (if it exists)
    
    
    if(exports.bytes_sent >= exports.size) {     // If we've sent all the pieces successfully
      if (success_callback) success_callback();  // Call success callback (if it exists)
      return;                                    // And we're done!
    }
    
    if (exports.abort) {                         // If user is aborting transfer
      if (error_callback) error_callback();      // Call error callback (if it exists)
      //MessageQueue.sendAppMessage({"JIGSAW_ABORT" : 0}, error_callback, error_callback);
      return;                                    // And exit
    }
    
    // Cut a piece to send
    exports.piece_size = exports.size - exports.bytes_sent > exports.piece_max_size ? exports.piece_max_size : exports.size - exports.bytes_sent;
    exports.piece = data.slice(exports.bytes_sent, exports.bytes_sent + exports.piece_size);
    // Send the piece and if successful, call this function again to send next piece
    MessageQueue.sendAppMessage({"JIGSAW_PIECE_INDEX": exports.bytes_sent, "JIGSAW_PIECE": exports.piece}, send_next_piece, error_callback);
  }

  // Prepare to send init and first piece
  exports.abort = false;
  exports.bytes_sent = 0;
  exports.size = data.length;
  // Cut the first piece to send
  exports.piece_size = exports.size > exports.piece_max_size ? exports.piece_max_size : exports.size;
  exports.piece = data.slice(0, exports.piece_size);
  // Send the piece and if successful, call send_next_piece to send next piece
  MessageQueue.sendAppMessage({"JIGSAW_INIT" : exports.size, "JIGSAW_PIECE": exports.piece}, send_next_piece, error_callback);
};



/*************************************************************************************************************************************/

/*
module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  exports.bytes_sent = 0;
  exports.size = data.length;
  
  (function send_piece() {
    // If this isn't the first piece
    if (exports.bytes_sent > 0) {
      if (exports.debug) console.log("Jigsaw JS: Successfully sent piece to watch!");
      if (piece_success_callback) piece_success_callback();  // Call piece success callback (if it exists)
    }

    // If we've sent all the pieces successfully, we're done!
    if(exports.bytes_sent >= exports.size) {
      if (exports.debug) console.log("Jigsaw JS: Successfully sent jigsaw to watch!");
      if (success_callback) success_callback();  // Call success callback (if it exists)
      return;
    }
    

    // Cut a piece to send
    exports.piece_size = exports.size - exports.bytes_sent > exports.piece_max_size ? exports.piece_max_size : exports.size - exports.bytes_sent;
    exports.piece = data.slice(exports.bytes_sent, exports.bytes_sent + exports.piece_size);
    var message = {};

    if(exports.bytes_sent === 0) { // If first piece, send piece with init
      if (exports.debug) console.log("Jigsaw JS: Starting new " + exports.size + " byte jigsaw...");
      message = {"JIGSAW_INIT" : exports.size, "JIGSAW_PIECE": exports.piece};
    } else { // else: isn't first piece. Send piece with where the piece goes
      message = {"JIGSAW_PIECE_INDEX": exports.bytes_sent, "JIGSAW_PIECE": exports.piece};
    }
    
    // Send piece and if successful, call this function again to send next piece
    if (exports.debug) console.log("Jigsaw JS: Sending " + exports.piece_size + " byte piece " +
                                   "[" + exports.bytes_sent + " to " + (exports.bytes_sent+exports.piece_size) + "]" +
                                   " of " + exports.size + " bytes...");
    exports.bytes_sent += exports.piece_size;
    MessageQueue.sendAppMessage(message, send_piece, error_callback);
  })();
};
*/

/*************************************************************************************************************************************/

/*
module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  function send_next_piece() {
    if (exports.debug) console.log("Jigsaw JS: Successfully sent piece to watch!");
    if (piece_success_callback) piece_success_callback();  // Call piece success callback (if it exists)

    // If we've sent all the pieces successfully, we're done!
    if(exports.bytes_sent >= exports.size) {
      if (exports.debug) console.log("Jigsaw JS: Successfully sent jigsaw to watch!");
      if (success_callback) success_callback();  // Call success callback (if it exists)
      return;
    }

    // Cut a piece to send
    exports.piece_size = exports.size - exports.bytes_sent > exports.piece_max_size ? exports.piece_max_size : exports.size - exports.bytes_sent;
    exports.piece = data.slice(exports.bytes_sent, exports.bytes_sent + exports.piece_size);

    // Send piece and if successful, call this function again to send next piece
    if (exports.debug) console.log("Jigsaw JS: Sending " + exports.piece_size + " byte piece " +
                                   "[" + exports.bytes_sent + " to " + (exports.bytes_sent+exports.piece_size) + "]" +
                                   " of " + exports.size + " bytes...");
    exports.bytes_sent += exports.piece_size;
    MessageQueue.sendAppMessage({"JIGSAW_PIECE_INDEX": exports.bytes_sent, "JIGSAW_PIECE": exports.piece}, send_next_piece, error_callback);
  }

  exports.bytes_sent = 0;
  exports.size = data.length;
  exports.piece_size = exports.size > exports.piece_max_size ? exports.piece_max_size : exports.size;
  exports.piece = data.slice(0, exports.piece_size);
  if (exports.debug) console.log("Jigsaw JS: Starting new " + exports.size + " byte jigsaw...");
  if (exports.debug) console.log("Jigsaw JS: Sending " + exports.piece_size + " byte piece " +
                                 "[" + exports.bytes_sent + " to " + exports.piece_size + "]" +
                                 " of " + exports.size + " bytes...");
  exports.bytes_sent += exports.piece_size;
  MessageQueue.sendAppMessage({"JIGSAW_INIT" : exports.size, "JIGSAW_PIECE": exports.piece}, send_next_piece, error_callback);

};
*/

/*************************************************************************************************************************************/

/*
module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  exports.bytes_sent = 0;
  exports.size = data.length;
  
  (function send_piece() {
    // If this isn't the first piece
    if (exports.bytes_sent > 0) {
      if (exports.debug) console.log("Jigsaw JS: Successfully sent piece to watch!");
      if (piece_success_callback) piece_success_callback();  // Call piece success callback (if it exists)
    }

    // If we've sent all the pieces successfully, we're done!
    if(exports.bytes_sent >= exports.size) {
      if (exports.debug) console.log("Jigsaw JS: Successfully sent jigsaw to watch!");
      if (success_callback) success_callback();  // Call success callback (if it exists)
      return;
    }
    

    // Cut a piece to send
    exports.piece_size = exports.size - exports.bytes_sent > exports.piece_max_size ? exports.piece_max_size : exports.size - exports.bytes_sent;
    exports.piece = data.slice(exports.bytes_sent, exports.bytes_sent + exports.piece_size);
    var message = {};

    if(exports.bytes_sent === 0) { // If first piece, send piece with init
      if (exports.debug) console.log("Jigsaw JS: Starting new " + exports.size + " byte jigsaw...");
      message = {"JIGSAW_INIT" : exports.size, "JIGSAW_PIECE": exports.piece};
    } else { // else: isn't first piece. Send piece with where the piece goes
      message = {"JIGSAW_PIECE_INDEX": exports.bytes_sent, "JIGSAW_PIECE": exports.piece};
    }
    
    // Send piece and if successful, call this function again to send next piece
    if (exports.debug) console.log("Jigsaw JS: Sending " + exports.piece_size + " byte piece " +
                                   "[" + exports.bytes_sent + " to " + (exports.bytes_sent+exports.piece_size) + "]" +
                                   " of " + exports.size + " bytes...");
    exports.bytes_sent += exports.piece_size;
    MessageQueue.sendAppMessage(message, send_piece, error_callback);
  })();
};
*/

/*************************************************************************************************************************************/

/*
module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  var index = 0;
  (function send_piece() {
    // If we've sent all the pieces successfully, we're done!
    if(index >= data.length) {
      if (exports.debug)
        console.log("Jigsaw JS: Successfully sent jigsaw to watch!");
      // Call success callback (if it exists)
      success_callback && success_callback();  // jshint ignore:line
      return;
    } else if (index > 0) {
      if (exports.debug)
        console.log("Jigsaw JS: Successfully sent piece to watch!");
      piece_success_callback && piece_success_callback();  // jshint ignore:line
    }
    
    var piece_size = data.length - index > exports.PIECE_MAX_SIZE ? exports.PIECE_MAX_SIZE : data.length - index;
    var piece = data.slice(index, index + piece_size);
    
    // Send piece and if successful, call this function again to send next piece
    if (index === 0) {  // If first piece, send piece with init
      if (exports.debug)
        console.log("Jigsaw JS: Starting new " + data.length + " byte jigsaw...");
      MessageQueue.sendAppMessage({"JIGSAW_INIT" : data.length, "JIGSAW_PIECE": piece}, send_piece, error_callback);
    } else {            // else: isn't first piece. Send piece with where the piece goes
      MessageQueue.sendAppMessage({"JIGSAW_PIECE_INDEX": index, "JIGSAW_PIECE": piece}, send_piece, error_callback);
    }
    
    if (exports.debug)
      console.log("Jigsaw JS: Sending " + piece_size + " byte piece [" + index + " to " + (index+piece_size) + "] of " + data.length + " bytes...");
    
    index += piece_size;
  })();
};
*/

/*************************************************************************************************************************************/

/*
module.exports.send_to_pebble = function(data, success_callback, error_callback, piece_success_callback) {
  var index = 0;
  (function send_piece() {
    if(index >= data.length) { // If we've sent all the pieces successfully, we're done!
      if (success_callback) success_callback();  // Call success callback (if it exists)
      return;
    } else if (index > 0) {
      if (piece_success_callback) piece_success_callback();
    }
    
    var piece_size = data.length - index > exports.PIECE_MAX_SIZE ? exports.PIECE_MAX_SIZE : data.length - index;
    var piece = data.slice(index, index + piece_size);
    
    // Send piece and if successful, call this function again to send next piece
    if (index === 0) {  // If first piece, send piece with init
      MessageQueue.sendAppMessage({"JIGSAW_INIT" : data.length, "JIGSAW_PIECE": piece}, send_piece, error_callback);
    } else {            // else: isn't first piece. Send piece with where the piece goes
      MessageQueue.sendAppMessage({"JIGSAW_PIECE_INDEX": index, "JIGSAW_PIECE": piece}, send_piece, error_callback);
    }
    index += piece_size;
  })();
};
*/

/*************************************************************************************************************************************/