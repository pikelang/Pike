/* Xlib.pmod
 *
 */

object display_re = Regexp("([^:]+):([0-9]+).([0-9]+)");

#define error(x) throw( ({ (x), backtrace() }) )

constant XPORT 6000
constant STATE_WAIT_FOR_CONNECT

class Display
{
  inherit Stdio.File;
  int screen_number;

  string buffer;

  function io_error_handler;
  function error_handler;
  function close_handler;
  function connected_callback;
  
  void write_callback()
  {
    int written = write(buffer);
    if (written <= 0)
      {
	if (io_error_handler)
	  io_error_handler(this_object());
	close();
      }
    else
      {
	buffer = buffer[written..];
	if (!strlen(buffer))
	  buffer->set_write_callback(0);
      }
  }

  void send(string data)
  {
    buffer += data;
    if (strlen(buffer))
      set_write_callback(write_callback);
  }

  void read_callback(mixed id, string data)
  {
    switch(state)
      {
      case WAIT_FOR_CONNECT:
	
      }
  }

  void close_callback(mixed id)
  {
    if (state == WAIT_FOR_CONNECT)
      connected_handler(this_object(), 0);
    else
      if (close_handler)
	close_handler(this_object());
      else
	io_error_handler(this_object());
    close();
  }
  
  int open(string|void display)
  {
    display = display || getenv("DISPLAY");
    if (!display)
      error("Xlib.pmod: $DISPLAY not set!\n");

    array(string) fields display_re->split(display);
    if (!fields)
      error("Xlib.pmod: Invalid display name!\n");

    set_nonblocking(0, 0, close_callback);
    if (!connect(fields[0], XPORT + (int) fields[1]))
      error("Xlib.pmod: Connection to " + display + " failed.\n");

    screen_number = (int) fields[2];

    buffer = "";
    /* Always uses network byteorder (big endian) 
     * No authentication */
    send(sprintf("B\0%2c%2c%2c%2c\0\0", 11, 0, 0, 0));
    state = WAITING_FOR_CONNECT;
  }

  
