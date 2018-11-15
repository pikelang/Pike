  inherit Tools.Hilfe.GenericAsyncHilfe;

  Stdio.File client;
  object debug_server;
  object current_object;
  array locals;
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


  protected void create(Stdio.File _client, object _debug_server, string desc, object _current_object, array _locals, array _bt)
  {
//	  werror("client: %O\n", client);
    debug_server = _debug_server;
 	client = _client;
	current_object = _current_object;
	locals = _locals;
    backtrace = _bt;

    client->write("Breakpoint on " + desc + "\n");
    ::create(client, client);
	//werror("Initialized\n");

    foreach((["locals": locals, "backtrace": backtrace, "current_object": current_object]); string h; mixed v)
    {
      variables[h] = v;
      types[h] = sprintf("%t", v);
    }

    m_delete(commands, "exit");
    m_delete(commands, "quit");
    commands->go = CommandGo();
    commands->step = CommandStep();
	
//    commands->bt = CommandBackTrace();
//    commands->backtrace = commands->bt;
//client->write("Initialized\n");

//	reset_evaluator();
  }

protected void _destruct()
{
  object key = debug_server->bp_lock->lock();
  debug_server->breakpoint_cond->signal();
  key = 0;
}
