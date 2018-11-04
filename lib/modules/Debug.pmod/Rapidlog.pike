//! Allows for rapid collection of logdata, which is then fed to the
//! real werror() at idle moments.
//! This allows for logging to occur with minimal timing interference.

#pike __REAL_VERSION__

#pragma dynamic_dot

#define POLLINTERVAL	1		// Polling interval in seconds
#define DRAINRATE	10		// Lines per second
#define MAXBUFENTRIES	512		// To keep the buffersize down

private array(string) fifo = ({});
private int pollinterval = POLLINTERVAL;
private int drainrate = DRAINRATE;
private int maxbufentries = MAXBUFENTRIES;

// Starts up the background thread.
private int idle = 0, options = (Thread.Thread(loop), 0);

//! Overloads the @[predef::werror()] function
//! to allow floods of logentries while introducing minimal latency.
//! Logs are buffered, and periodically flushed from another thread.
//! If the arrival rate of logs is excessive, it simply skips
//! part of the logs to keep up.
//!
//! @note
//!   When parts are skipped, records are skipped in whole and will never be
//!   split up.
//!
//! @seealso
//!  @[werror_options()]
void werror(string format, mixed ... args) {
  idle = 0;
  fifo += ({sizeof(args) ? sprintf(format, @args) : format});
}

//! @param options
//!  Defaults to @expr{0@}, reserved for future enhancements.
//! @param pollinterval
//!  Pollinterval in seconds for the log-flush thread; defaults to @expr{1@}.
//!  Logs will only be flushed out, if during the last pollinterval
//!  no new logs have been added.
//! @param drainrate
//!  Maximum number of lines per second that will be dumped to stderr.
//!  Defaults to @expr{10@}.
//! @param maxbufentries
//!  Maximum number of buffered logrecords which have not been flushed to stderr
//!  yet.  If this number is exceeded, the oldest @expr{maxbufentries/2@} entries
//!  will be skipped; a notice to that effect is logged to stderr.
//!
//! @seealso
//!  @[werror()]
void werror_options(int options, void|int pollinterval, void|int drainrate,
 void|int maxbufentries) {
  this::options = options;
  if (pollinterval) {
    this::pollinterval = pollinterval;
    if (drainrate) {
      this::drainrate = drainrate;
      if (maxbufentries)
        this::maxbufentries = maxbufentries;
    }
  }
}

private void loop() {
  for(;;) {
    idle = 1;
    sleep(pollinterval);
    if (idle) {
      int i, lines = 0;
      foreach (fifo; i; string msg) {
        predef::werror(msg);
        if ((lines += String.count(msg, "\n")) >= pollinterval * drainrate)
          break;
      }
      fifo = fifo[i + 1 ..];
    } else if (sizeof(fifo) > maxbufentries) {
      int lines = 0;
      foreach (fifo[.. maxbufentries/2 - 1]; ; string msg)
        lines += String.count(msg, "\n");
      fifo = fifo[maxbufentries/2 ..];
      predef::werror("[... Skipping %d entries = %d lines ...]\n",
       maxbufentries/2, lines);
    }
  }
}
