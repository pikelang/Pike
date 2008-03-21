#pike 7.8
/*
 * $Id: Pipe.pmod,v 1.1 2008/03/21 13:05:39 grubba Exp $
 *
 * Emulation of the Pipe C-module from Pike 7.6 and earlier.
 *
 * 2008-03-20 Henrik Grubbström
 */

array _pipe_debug();

//! Concatenation pipe.
class pipe
{
  private {
    function(mixed:mixed) done_callback;
    function(mixed, object:mixed) output_closed_callback;
    mixed id;

    int active;
    int sent;

    Stdio.File output_buffer;
    int output_buffer_pos;
    int output_buffer_bytes;

    ADT.Queue input_queue = ADT.Queue();
    ADT.Queue output_queue = ADT.Queue();

    void send_inputs(int bytes, object output)
    {
      sent += bytes;

      array(string) headers = ({});
      object file;
      string|object input;
      while (stringp(input = input_queue->get())) {
	headers += ({ [string]input });
      }
      file = input;
      if (file || sizeof(headers)) {
	Stdio.sendfile(headers, file, -1, -1, 0, output, send_inputs, output);
      } else {
	if (output == output_buffer) output_buffer_bytes = sent;
	next_output();
      }
    }
    void next_output(int|void bytes)
    {
      sent += bytes;

      object output = output_queue->get();
      if (output) {
	if (output_buffer) {
	  // FIXME: With a true sendfile() we could actually send to all
	  //        the outputs in parallel here. Unfortunately
	  //        Stdio.nbsendfile() affects and is affected by the seek
	  //        position of the input file.
	  Stdio.sendfile(0, output_buffer, output_buffer_pos,
			 output_buffer_bytes, 0, output, next_output);
	} else {
	  send_inputs(0, output);
	}
      } else {
	finish();
      }
    }
  }

  //! Add an input file.
  void input(object obj)
  {
    obj->set_id(0);
    input_queue->put(obj);
  }

  //! Add an input string.
  void write(string bytes)
  {
    input_queue->put(bytes);
  }

  //! Add an output file object.
  void output(object obj, int|void start_pos)
  {
    if (!output_buffer && obj->dup2 && obj->tell) {
      Stdio.Stat st = obj->stat && obj->stat();
      // FIXME: Catch errors?
      if (st && st->isreg) {
	Stdio.File buf = Stdio.File();
	if (obj->dup2(buf)) {
	  // NOTE: In the old implementation the inputs were copied
	  //       to the buffer here. In this implementation we have
	  //       moved this to start()-time.
	  output_buffer = buf;
	  output_buffer_pos = obj->tell();

	  // Obscure compat stuff...
	  obj->set_id(0);
	  return;
	}
      }
    }
    output_queue->put(obj);
    // Obscure compat stuff...
    obj->set_id(obj);
  }

  //! Set the callback function to be called when all the outputs
  //! have been sent.
  void set_done_callback(void|function(mixed:mixed) done_cb,
			 void|mixed id)
  {
    done_callback = done_cb;
    if (query_num_arg() > 1) this_program::id = id;
  }

  //! Set the callback function to be called when one of the outputs has
  //! been closed from the other side.
  //!
  //! @note
  //!   Not currently used by the Pike 7.7 implementation.
  void set_output_closed_callback(void|function(mixed, object:mixed) close_cb,
				  void|mixed id)
  {
    // FIXME: Not actually used in this implementation!
    output_closed_callback = close_cb;
    if (query_num_arg() > 1) this_program::id = id;
  }

  //! Terminate and reinitialize the pipe.
  void finish()
  {
    if (done_callback) {
      done_callback(id);
      if (!this_object()) return;	// Destructed by done_callback().
    }
    __INIT();	// close_and_free_everything().
  }

  //! Start sending the input(s) to the output(s).
  void start()
  {
    if (active) error("Pipe already started.\n");
    active = 1;
    if (output_buffer) {
      output_buffer->seek(output_buffer_pos);
      send_inputs(0, output_buffer);
    } else {
      next_output();
    }
  }

  void _output_close_callback(int);
  void _input_close_callback(int);
  void _output_write_callback(int);
  void _input_read_callback(int);

  //! Return the version of the module.
  string version() { return "PIPE ver 2.0 (emulated)"; }

  //! Return the number of bytes sent.
  int bytes_sent()
  {
    return sent;
  }
}
