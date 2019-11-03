#define DEBUG(X, Y ...) do { if (debug) werror(X, Y); } while (0)
#define DEFAULT_PORT 3333

import .DebugAdapterProtocol;
array(mixed) stackinfo;
bool debug = false;
bool im_a_server = true;
Stdio.Buffer obuf = Stdio.Buffer();
bool attach = false;

// TODO: pack this into a nice class
int var_ref_ct;
mapping (int(1..):array(mixed)) vars_in_frame;

void write_event(ProtocolMessage msg, bool|void violently) {
    string msg_str = encode_message(msg);

    DEBUG("\nSending event: \n%s\n", msg_str);

    if (im_a_server)
        if (violently)
            breakpoint_client->write(msg_str);
        else
            obuf->add(msg_str);
    else obuf->write(msg_str);
}

void write_response(ProtocolMessage msg) {
    string msg_str = encode_message(msg);

    DEBUG("\nSending response: \n%s\n", msg_str);

    if (im_a_server) breakpoint_client->write(msg_str);
    else obuf->write(msg_str);
}

void handle_initialize_request(mixed msg) {
    InitializeRequest req = InitializeRequest(msg);
    InitializeResponse res = InitializeResponse();

    res->body->supports_configuration_done_request = 1;
    res->body->supports_set_variable = 1;
    res->request_seq = req->seq;
    res->success = 1;

    write_response(res);

    InitializedEvent evt = InitializedEvent();

    write_event(evt);
}

void handle_attach_request(mixed msg) {
    AttachRequest req = AttachRequest(msg);
    AttachResponse res = AttachResponse();

    res->request_seq = req->seq;
    res->success = 1;

    attach = true;

    write_response(res);
}

void handle_continue_request(mixed msg) {
    ContinueRequest req = ContinueRequest(msg);
    ContinueResponse res = ContinueResponse();

    res->request_seq = req->seq;

	set_mode(CONTINUE);
    object key;

    DEBUG("Resuming.\n");

    if(breakpoint_cond) {
        key = bp_lock->lock();
        breakpoint_cond->signal();
        key = 0;
        breakpoint_cond = 0;
    }
    if(wait_cond) {
        key = wait_lock->lock();
        wait_cond->signal();
        key = 0;
        wait_cond = 0;
    }

    res->success = 1;

    write_response(res);

}


void handle_launch_request(mixed msg) {
    LaunchRequest req = LaunchRequest(msg);
    LaunchResponse res = LaunchResponse();

    res->request_seq = req->seq;
    res->success = 1;

    write_response(res);
}

void handle_evaluate_request(mixed msg) {
    EvaluateRequest req = EvaluateRequest(msg);
    Response res = EvaluateResponse();

    res->request_seq = req->seq;
    res->success = 1;
    res->body = ([
        "result" : "EvaluateResponse echo: " + req->?arguments->?expression,
        "variablesReference" : 0
    ]);

    write_response(res);
}

void handle_threads_request(mixed msg) {
    // TODO
    Request req = Request(msg);
    ThreadsResponse res = ThreadsResponse();

    DAPThread dt = DAPThread();
    dt->id = 1;
    dt->name = "dummy";

    res->body = (["threads" : ({ dt })]);

    res->request_seq = req->seq;
    res->success = 1;

    write_response(res);
}

void handle_configuration_done_request(mixed msg) {
    // TODO
    Request req = Request(msg);
    Response res = Response();

    res->request_seq = req->seq;
    res->success = 1;
    res->command = req->command;


    write_response(res);

    if (attach) return;

    StoppedEvent evt = StoppedEvent();
    evt->body = (["reason" : "entry", "threadId" : 1]);
    write_event(evt);
}

void handle_stack_trace_request(mixed msg) {
    Request req = StackTraceRequest(msg);
    Response res = StackTraceResponse();

    res->request_seq = req->seq;
    res->success = 1;
    array(mapping(string:mixed)) frames = ({});
    int frameId;

    var_ref_ct = 0;
    vars_in_frame = ([]);

    if (stackinfo) {
        foreach(stackinfo, mixed frame) {
            frames += ({
                ([
                    "id" : frameId++,
                    "line": frame[1],
                    "column": 0,
                    "name": sprintf("%O", frame[2]),
                    "source": ([
                        "path": frame[0],
                        "name": (frame[0] / "/")[-1]
                        ])
                ])
            });
        }
    }
    else {
        // seems like VSCode doesn't handle empty stack trace properly
        // that's why this dummy frame is added
        frames += ({
            ([
                "id" : 0,
                "line": 0,
                "column": 0,
                "name": "dummy frame"
            ])
        });
    }
    res->body = ([
        "stackFrames" : frames,
        "totalFrames" : sizeof(frames)
    ]);

    write_response(res);
}

void handle_scopes_request(mixed msg) {
    Request req = ScopesRequest(msg);
    Response res = ScopesResponse();
    int frame_id = req->arguments->frame_id;
    res->request_seq = req->seq;
    res->success = 1;

    array(mixed) vars_in_scope = ({});
    mixed frame = stackinfo[frame_id];

    for (int i = 0; i < sizeof(frame->var_names); ++i) {
        vars_in_scope += ({
            ({
                frame->var_types[i],
                frame->var_names[i],
                frame->vars[i]
            })
        });
    }

    vars_in_frame += ([++var_ref_ct : vars_in_scope]);

    res->body = ([
        "scopes" : ({
            ([
                "name" : "Current scope",
                "variablesReference" : var_ref_ct,
                "expensive" : false
            ])
        })
    ]);

    write_response(res);
}

void handle_variables_request(mixed msg) {
    Request req = VariablesRequest(msg);
    Response res = VariablesResponse();

    int var_ref = req->arguments->variables_reference;

    array(mapping(string:mixed)) vars = ({});
    array(mixed) members = vars_in_frame[var_ref];

    foreach(members, mixed var) {
        // count var ref for complex types
        int new_var_ref = 0;
        array children = ({});
        if (arrayp(var[2]) || mappingp(var[2]) || multisetp(var[2]) || objectp(var[2])) {
            new_var_ref = ++var_ref_ct;
            // mapping and object shouldn't be trated identically
            // probably it's a reason for the backtrace frame bug
            if (mappingp(var[2]) || objectp(var[2])) {
                foreach(indices(var[2]), mixed v) {
                    children += ({
                        ({
                            _typeof(var[2][v]),
                            v,
                            var[2][v]
                        })
                    });
                }
            }
            else if (arrayp(var[2])) {
                int idx;
                foreach(var[2], mixed v) {
                    children += ({
                        ({
                            _typeof(v),
                            (string) idx++,
                            v
                        })
                    });
                }
            }
            else if (multisetp(var[2])) {
                int idx;
                foreach(indices(var[2]), mixed v) {
                    children += ({
                        ({
                            _typeof(v),
                            (string) idx++,
                            v
                        })
                    });
                }
            }
            vars_in_frame += ([
                new_var_ref : children
            ]);
        }
        vars += ({
            ([
                "type" : sprintf("%O", var[0]),
                "name" : callablep(var[1]) ? sprintf("%O", var[1]): var[1],
                "value" : new_var_ref? "" : sprintf("%O", var[2]),
                "variablesReference" : new_var_ref,
            ])
        });
    }

    res->request_seq = req->seq;
    res->success = 1;
    res->body = ([
        "variables" : vars
    ]);

    write_response(res);
}

void handle_action_request(mixed msg) {
    // TODO
    handle_request_generic(msg);

    StoppedEvent evt = StoppedEvent();
    evt->body = (["reason" : "pause", "threadId" : 1]);
    sleep(0.5);
    write_event(evt);
}

void handle_breakpoints_request(mixed msg) {
    // TODO
    string path = msg->arguments->source->path;
    array(int) lines = msg->arguments->lines;

    // The main program is not indexed with its original path, it is indexed
    // with "/main". See code in 'master.pike.in' for more detail.
    if ( path == master()->main_path )
        path = "/main";

    foreach(Debug.Debugger.breakpoints; object bp;) {
        catch { bp->disable(); };
    }

    foreach(lines, int line) {
        mixed e = catch {
            object bp = Debug.Debugger.add_breakpoint(path, line);
            if(!bp->enable()) {
	            program prog = bp->get_program();
                // try set_program on nested programs
                foreach(indices(prog), string pname)
                    if(programp(prog[pname]) && bp->set_program(prog[pname]))
                        break;
	        }
        };
    }

    Request req = Request(msg);
    Response res = Response();

    res->request_seq = req->seq;
    res->success = 1;
    res->command = req->command;
    res->body = ([
        "breakpoints" : ({}) // TODO
    ]);

    write_response(res);
}

void handle_set_variable_request(mixed msg) {
    Request req = SetVariableRequest(msg);
    Response res = SetVariableResponse();

    res->request_seq = req->seq;
    res->success = 1;

    string name = req->arguments->name;
    int value = (int) req->arguments->value;
    // TODO: frame_id instead
    int var_idx = search(stackinfo[0]->var_names, name);
    stackinfo[0]->set_local(var_idx, value);
    res->body->value = req->arguments->value;

    write_response(res);
}

void handle_request_generic(mixed msg) {
    Request req = Request(msg);
    Response res = Response();

    res->request_seq = req->seq;
    res->success = 1;
    res->command = req->command;

    write_response(res);
}

// breakpointing support
Stdio.Port breakpoint_port;
int breakpoint_port_no = 3333;
Stdio.File breakpoint_client;

constant SINGLE_STEP = 1;
constant CONTINUE = 0;

object breakpoint_cond;
object evaluate_cond;
object wait_cond;
object bp_lock = Thread.Mutex();
object wait_lock = Thread.Mutex();
object breakpoint_hilfe;
object bpbe;
object bpbet;

int wait_seconds;
int mode = .BreakpointHilfe.CONTINUE;

public void set_mode(int m) {
	mode = m;
}

// pause up to wait_seconds for a debugger to connect before continuing
public void load_breakpoint()
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

public int do_breakpoint(string file, int line, string opcode, object current_object)
{
  if(!breakpoint_client) return 0;

  stackinfo = backtrace(1)[1..];
  // the object needs to be 'touched' in this scope, else it's optimized-out
  Stdio.File fd = Stdio.File("/dev/null");
  foreach(stackinfo, mixed frame) {
      fd->write("%O %O %O", frame->var_names, frame->var_types, frame->vars);
  }
  StoppedEvent evt = StoppedEvent();
  evt->body->reason = "breakpoint";
  evt->body->threadId = 1;
  write_event(evt, true);
  // TODO: what do we do if a breakpoint is hit and we already have a running hilfe session,
  // such as if a second thread encounters a bp?
  object key = bp_lock->lock();
  if (!breakpoint_cond)
    breakpoint_cond = Thread.Condition();
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

  breakpoint_client->set_backend(bpbe);
  breakpoint_client->set_nonblocking(breakpoint_read, breakpoint_write, breakpoint_close);
}

private void breakpoint_close(mixed data)
{
    DEBUG("\nClose callback. Shutting down.\n");
    exit(0);
}


private void breakpoint_write(mixed id)
{
    if (im_a_server) {
        string m = obuf->read();
        if (sizeof(m)) breakpoint_client->write(m);
    }
}

private void breakpoint_read(mixed id, string data)
{
    mixed msg = Standards.JSON.decode(parse_request(data));
    DEBUG("\nReceived request: \n%s\n", data);
    switch (msg[?"command"]) {
        case "initialize":
            handle_initialize_request(msg);
            break;
        case "attach":
            handle_attach_request(msg);
            break;
        case "launch":
            handle_launch_request(msg);
            break;
        case "evaluate":
            handle_evaluate_request(msg);
            break;
        case "threads":
            handle_threads_request(msg);
            break;
        case "configurationDone":
            handle_configuration_done_request(msg);
            break;
        case "stackTrace":
            handle_stack_trace_request(msg);
            break;
        case "scopes":
            handle_scopes_request(msg);
            break;
        case "continue":
            handle_continue_request(msg);
            break;
        case "next":
        case "stepIn":
        case "stepOut":
            handle_action_request(msg);
            break;
        case "setBreakpoints":
            handle_breakpoints_request(msg);
            break;
        case "variables":
            handle_variables_request(msg);
            break;
        case "setVariable":
            handle_set_variable_request(msg);
            break;
        default:
            handle_request_generic(msg);
    }
}
