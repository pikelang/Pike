
// breakpointing support
Stdio.Port breakpoint_port;
int breakpoint_port_no = 3333;
Stdio.File breakpoint_client;

object breakpoint_cond;
object wait_cond;
object bp_lock = Thread.Mutex();
object wait_lock = Thread.Mutex();
object breakpoint_hilfe;
object bpbe;
object bpbet;

int mode = .BreakpointHilfe.CONTINUE; 

public void set_mode(int m) {
	mode = m;
}

// pause up to wait_seconds for a debugger to connect before continuing
public void load_breakpoint(int wait_seconds)
{
    bpbe = Pike.Backend();
    bpbet = Thread.Thread(lambda(){ do { catch(bpbe(1000.0)); } while (1); });
//    bpbet->set_thread_name("Breakpoint Thread");

    werror("Starting Debug Server on port %d.\n", breakpoint_port_no);
    breakpoint_port = Stdio.Port(breakpoint_port_no, handle_breakpoint_client);
    breakpoint_port->set_backend(bpbe);
	Debug.Debugger.set_debugger(this);
	
	// wait of -1 is infinite
	if(wait_seconds > 0) 
  	  bpbe->call_out(wait_timeout, wait_seconds);
	  
	if(wait_seconds != 0) {
		werror("waiting up to " + wait_seconds + " for debugger to connect\n");
	    object key = wait_lock->lock();
	    wait_cond = Thread.Condition();
	    wait_cond->wait(key);
	    key = 0;
	}
}

void wait_timeout() {
	werror("debugger connect wait timed out.\n");
    object key = wait_lock->lock();
    wait_cond->signal();
    key = 0;
}

// method that is called upon hitting a breakpoint: causes a waiting debugger connection to
// receive a hilfe prompt.
public int do_breakpoint(string file, int line, string opcode, object current_object, array bt)
{
  if(!breakpoint_client) return 0;

  // TODO: what do we do if a breakpoint is hit and we already have a running hilfe session,
  // such as if a second thread encounters a bp?
  object key = bp_lock->lock();
  breakpoint_cond = Thread.Condition();
  bpbe->call_out(lambda(){breakpoint_hilfe = .BreakpointHilfe(breakpoint_client, this, file + ":" + line, current_object, bt);}, 0);
  werror("Hilfe started for Breakpoint on %s at %s.\n", opcode, file + ":" + line);
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

  // if we are holding pike execution until a debugger connects, remove the timeout.  
  if(wait_cond) {
	bpbe->remove_call_out(wait_timeout);
  }

  breakpoint_client->write("Welcome to the Pike Debugger.\n");

  // if we are waiting for a debugger to connect on startup, throw that debugger into a live hilfe
  // otherwise, wait for a breakpoint to be hit.
  if(wait_cond) {
      bpbe->call_out(lambda(){breakpoint_hilfe = .BreakpointHilfe(breakpoint_client, this);}, 0);
  }

  breakpoint_hilfe->read_callback(0, "");
  breakpoint_client->set_backend(bpbe);
  breakpoint_client->set_nonblocking(breakpoint_read, breakpoint_write, breakpoint_close);
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

