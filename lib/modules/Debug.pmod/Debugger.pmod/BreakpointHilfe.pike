  inherit Tools.Hilfe.GenericAsyncHilfe;

  Stdio.File client;
  object debug_server;
  object current_object;
  array backtrace;

  constant SINGLE_STEP = 1;
  constant CONTINUE = 0;
  
  void print_version()
  {
  }

  class CommandGo
  {
    inherit Tools.Hilfe.Command;
    string help(string what) { return "Resume request."; }

    void exec(Tools.Hilfe.Evaluator e, string line, array(string) words,
            array(string) tokens) {
    e->safe_write("Resuming.\n");
	debug_server->set_mode(CONTINUE);
    destruct(e);
    }
  }
  
  class CommandStep
  {
    inherit Tools.Hilfe.Command;
    string help(string what) { return "Step request."; }

    void exec(Tools.Hilfe.Evaluator e, string line, array(string) words,
            array(string) tokens) {
    e->safe_write("Stepping.\n");
	debug_server->set_mode(SINGLE_STEP);
    destruct(e);
    }
  }
 
  class CommandBackTrace
  {
    inherit Tools.Hilfe.Command;
    string help(string what) { return "Display a backtrace that got us here."; }

    void exec(Tools.Hilfe.Evaluator e, string line, array(string) words,
            array(string) tokens) {
    e->safe_write(describe_backtrace(e->backtrace[1..]));
    }
    
  }

  void read_callback(mixed id, string s)
  {
    s = replace(s, "\r\n", "\n");
    inbuffer+=s;
    if(has_suffix(inbuffer, "\n")) inbuffer = inbuffer[0.. sizeof(inbuffer)-2];
    foreach(inbuffer/"\n",string s)
    {
      inbuffer = inbuffer[sizeof(s)+1..];
      add_input_line(s);
      write(state->finishedp() ? "> " : ">> ");
    }
  }

  protected variant void create(Stdio.File _client, object _debug_server)
  {
    debug_server = _debug_server;
   	client = _client;
	
    ::create(client, client);
	
    m_delete(commands, "exit");
    m_delete(commands, "quit");
    commands->go = CommandGo();
  }

  protected variant void create(Stdio.File _client, object _debug_server, string desc, object _current_object, array _bt)
  {
//	  werror("client: %O\n", client);
    debug_server = _debug_server;
 	client = _client;
	current_object = _current_object;
    backtrace = _bt;
	
    client->write("Breakpoint on " + desc + "\n");
    ::create(client, client);
	//werror("Initialized\n");

    foreach((["backtrace": backtrace, "current_object": current_object]); string h; mixed v)
    {
      variables[h] = v;
      types[h] = sprintf("%t", v);
    }

    m_delete(commands, "exit");
    m_delete(commands, "quit");
    commands->go = CommandGo();
    commands->step = CommandStep();
 //   commands->set_local = CommandSetLocal();
	
    commands->bt = CommandBackTrace();
    commands->backtrace = commands->bt;
  }

protected void _destruct()
{
  object key;

  // when we are created by the debug server, a condition variable will be waited on.
  // signalling that condition variable causes execution to continue, which we want to do
  // as this hilfe object is destroyed by a continue or step command.

  // the breakpoint condition variable exists when a breakpoint has been reached and execution
  // in a thread is suspended for the debugger. signalling this variabnle causes the thread to
  // move on from the breakpoint.
  if(debug_server->breakpoint_cond) {
    key = debug_server->bp_lock->lock();
    debug_server->breakpoint_cond->signal();
    key = 0;
  }
  
  // the wait condition variable exists when the pike interpreter is waiting on startup for a
  // debugger to connect. when we exit from this situation, we are permitting the master object
  // to continue executing
  if(debug_server->wait_cond) {
    key = debug_server->wait_lock->lock();
    debug_server->wait_cond->signal();
    key = 0;
    debug_server->wait_cond = 0;
  }
}
