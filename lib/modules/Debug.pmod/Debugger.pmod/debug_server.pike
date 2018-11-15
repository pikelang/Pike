
// breakpointing support
Stdio.Port breakpoint_port;
int breakpoint_port_no = 3333;
Stdio.File breakpoint_client;
object bp_key;
object breakpoint_cond;
object bp_lock = Thread.Mutex();
object breakpoint_hilfe;
object bpbe;
object bpbet;

int mode = .BreakpointHilfe.CONTINUE; 

public void set_mode(int m) {
	mode = m;
}

public void load_breakpoint()
{
	bpbe = Pike.Backend();
    bpbet = Thread.Thread(lambda(){ do { catch(bpbe(1000.0)); } while (1); });
//    bpbet->set_thread_name("Breakpoint Thread");

    werror("Starting Debug Server on port %d.\n", breakpoint_port_no);
    breakpoint_port = Stdio.Port(breakpoint_port_no, handle_breakpoint_client);
    breakpoint_port->set_backend(bpbe);
	Debug.Debugger.set_debugger(this);
}


//! trigger a breakpoint in execution, if breakpointing is enabled.
//! @param desc
//!   description of breakpoint, to be passed to breakpoint client
//! @param state
//!   a mapping of data to make available for query at breakpoint client
//!

/*
public void breakpoint(string desc, mapping state)
{
  if(config["application"] && config["application"]["breakpoint"])
  {
    do_breakpoint(desc, state, backtrace());
  }
  else
  {
    logger->debug("breakpoints disabled, skipping breakpoint set from %O",
                 backtrace()[-2][2]);
  }
  return 0;
}
*/

public int do_breakpoint(string file, int line, string opcode, object current_object, array locals, array bt)
{
  if(!breakpoint_client) return 0;
  object key = bp_lock->lock();
  breakpoint_cond = Thread.Condition();
  bpbe->call_out(lambda(){breakpoint_hilfe = .BreakpointHilfe(breakpoint_client, this, file + ":" + line, current_object, locals, bt);}, 0);
  werror("Hilfe started for Breakpoint on %s at %s.", opcode, file + ":" + line);
  breakpoint_cond->wait(key);
  key = 0;
  // now, we must wait for the hilfe session to end.
  return mode;
}

private void handle_breakpoint_client(int id)
{
  if(breakpoint_client) {
    breakpoint_port->accept()->close();
  }
  else breakpoint_client = breakpoint_port->accept();
  breakpoint_client->write("Welcome to the Pike Debugger.\n");
  breakpoint_hilfe->read_callback(0, "");
  breakpoint_client->set_backend(bpbe);
  breakpoint_client->set_nonblocking(breakpoint_read, breakpoint_write, breakpoint_close);
  breakpoint_client->write("Welcome to the Pike Debugger.\n");
  breakpoint_hilfe->read_callback(0, "");
}

private void breakpoint_close()
{
  breakpoint_hilfe = 0;
  breakpoint_client = 0;
}

private void breakpoint_write(int id)
{
}

private void breakpoint_read(int id, string data)
{
}

