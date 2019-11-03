//
// Base Protocol, DAP's communication protocol helper functions:
//

string parse_request(string data) {
    Stdio.Buffer ibuf = Stdio.Buffer();
    ibuf->add(data);
    int length;
    [length] = ibuf->sscanf("Content-Length: %d\r\n\r\n");

    return ibuf->read(length);
}

string encode_message(ProtocolMessage msg) {
    String.Buffer sbuf = String.Buffer();

    string enc = Standards.JSON.encode(msg);
    sbuf->sprintf("Content-Length: %d\r\n\r\n%s", sizeof(enc), enc);
    return sbuf->get();
}

//
// Debug Adapter Protocol
//

class JsonEncodable {
    protected mapping(string:mixed) to_json();

    string encode_json() {
        return Standards.JSON.encode(to_json());
    }
}

// Base class of requests, responses, and events.
class ProtocolMessage {
    inherit JsonEncodable;

    int    seq;  // json: "seq"
    string type; // json: "type"

    protected void create(mixed|void json) {
        if (!json) return;

        seq = json["seq"];
        type = json["type"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "seq" : seq,
            "type" : type,
        ]);
    }
}

// A debug adapter initiated event.
class Event {
    inherit ProtocolMessage;

    mapping(string:mixed) body; // json: "body"
    string event;               // json: "event"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
            event = json["event"];
        }

        type = "event";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "body" : body,
            "event" : event,
        ]);

        return json;
    }
}

// A client or debug adapter initiated request.
class Request {
    inherit ProtocolMessage;

    string command; // json: "command"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            command = json["command"];
        }

        type = "request";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "command" : command,
        ]);

        return json;
    }
}

// Response for a request.
class Response {
    inherit ProtocolMessage;

    mixed  body;        // json: "body"
    string command;     // json: "command"
    string message;     // json: "message"
    int    request_seq; // json: "request_seq"
    int    success;     // json: "success"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
            command = json["command"];
            message = json["message"];
            request_seq = json["request_seq"];
            success = json["success"];
        }

        type = "response";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "body" : body,
            "command" : command,
            "message" : message,
            "request_seq" : request_seq,
            "success" : success,
        ]);

        return json;
    }
}

// The event indicates that some information about a breakpoint has changed.
class BreakpointEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": "",
                "breakpoint": Breakpoint()
            ]);
        }

        event = "breakpoint";
    }
}

// The event indicates that one or more capabilities have changed.
// Since the capabilities are dependent on the frontend and its UI, it might not be possible
// to change that at random times (or too late).
// Consequently this event has a hint characteristic: a frontend can only be expected to
// make a 'best effort' in honouring individual capabilities but there are no guarantees.
// Only changed capabilities need to be included, all other capabilities keep their values.
class CapabilitiesEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": "",
                "capabilities": Capabilities()
            ]);
        }

        event = "capabilities";
    }
}

// The event indicates that the execution of the debuggee has continued.
// Please note: a debug adapter is not expected to send this event in response to a request
// that implies that execution continues, e.g. 'launch' or 'continue'.
// It is only necessary to send a 'continued' event if there was no previous request that
// implied this.
class ContinuedEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "threadId": 0
            ]);
        }

        event = "continued";
    }
}

// The event indicates that the debuggee has exited and returns its exit code.
class ExitedEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "exitCode": 0
            ]);
        }

        event = "exited";
    }
}

// This event indicates that the debug adapter is ready to accept configuration requests
// (e.g. SetBreakpointsRequest, SetExceptionBreakpointsRequest).
// A debug adapter is expected to send this event when it is ready to accept configuration
// requests (but not before the 'initialize' request has finished).
// The sequence of events/requests is as follows:
// - adapters sends 'initialized' event (after the 'initialize' request has returned)
// - frontend sends zero or more 'setBreakpoints' requests
// - frontend sends one 'setFunctionBreakpoints' request
// - frontend sends a 'setExceptionBreakpoints' request if one or more
// 'exceptionBreakpointFilters' have been defined (or if 'supportsConfigurationDoneRequest'
// is not defined or false)
// - frontend sends other future configuration requests
// - frontend sends one 'configurationDone' request to indicate the end of the configuration.
class InitializedEvent {
    inherit Event;

    protected void create (mixed|void json) {

        if (json) ::create(json);
        else ::create();

        event = "initialized";
    }
}

// The event indicates that some source has been added, changed, or removed from the set of
// all loaded sources.
class LoadedSourceEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": "",
                "source": Source()
            ]);
        }

        event = "loadedSource";
    }
}

// The event indicates that some information about a module has changed.
class ModuleEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": "",
                "module": Module()
            ]);
        }

        event = "module";
    }
}

// The event indicates that the target has produced some output.
class OutputEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "output": ""
            ]);
        }

        event = "output";
    }
}

// The event indicates that the debugger has begun debugging a new process. Either one that
// it has launched, or one that it has attached to.
class ProcessEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "name": ""
            ]);
        }

        event = "process";
    }
}

// The event indicates that the execution of the debuggee has stopped due to some condition.
// This can be caused by a break point previously set, a stepping action has completed, by
// executing a debugger statement etc.
class StoppedEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": ""
            ]);
        }

        event = "stopped";
    }
}

// The event indicates that debugging of the debuggee has terminated. This does **not** mean
// that the debuggee itself has exited.
class TerminatedEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else ::create();

        event = "terminated";
    }
}

// The event indicates that a thread has started or exited.
class ThreadEvent {
    inherit Event;

    protected void create (mixed|void json) {
        if (json) ::create(json);
        else {
            ::create();
            body = ([
                "reason": "",
                "threadId": 0
            ]);
        }

        event = "thread";
    }
}

// The 'initialize' request is sent as the first request from the client to the debug
// adapter in order to configure it with client capabilities and to retrieve capabilities
// from the debug adapter.
// Until the debug adapter has responded to with an 'initialize' response, the client must
// not send any additional requests or events to the debug adapter. In addition the debug
// adapter is not allowed to send any requests or events to the client until it has
// responded with an 'initialize' response.
// The 'initialize' request may only be sent once.
class InitializeRequest {
    inherit Request;

    InitializeRequestArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = InitializeRequestArguments(json["arguments"]);
        }
        else {
            ::create();
            arguments = InitializeRequestArguments();
        };

        command = "initialize";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'initialize' request.
class InitializeRequestArguments {
    inherit JsonEncodable;

    string adapter_id;                       // json: "adapterID"
    string client_id;                        // json: "clientID"
    string client_name;                      // json: "clientName"
    int    columns_start_at1;                // json: "columnsStartAt1"
    int    lines_start_at1;                  // json: "linesStartAt1"
    string locale;                           // json: "locale"
    string path_format;                      // json: "pathFormat"
    int    supports_run_in_terminal_request; // json: "supportsRunInTerminalRequest"
    int    supports_variable_paging;         // json: "supportsVariablePaging"
    int    supports_variable_type;           // json: "supportsVariableType"

    protected void create(mixed|void json) {
        if (!json) return;

        adapter_id = json["adapterID"];
        client_id = json["clientID"];
        client_name = json["clientName"];
        columns_start_at1 = json["columnsStartAt1"];
        lines_start_at1 = json["linesStartAt1"];
        locale = json["locale"];
        path_format = json["pathFormat"];
        supports_run_in_terminal_request = json["supportsRunInTerminalRequest"];
        supports_variable_paging = json["supportsVariablePaging"];
        supports_variable_type = json["supportsVariableType"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            // no need to explicitly disable options
            //
            // "adapterID" : adapter_id,
            // "clientID" : client_id,
            // "clientName" : client_name,
            // "columnsStartAt1" : columns_start_at1,
            // "linesStartAt1" : lines_start_at1,
            // "locale" : locale,
            // "pathFormat" : path_format,
            // "supportsRunInTerminalRequest" : supports_run_in_terminal_request,
            // "supportsVariablePaging" : supports_variable_paging,
            // "supportsVariableType" : supports_variable_type,
        ]);
    }
}

// Response to 'initialize' request.
class InitializeResponse {
    inherit Response;

    Capabilities body;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = Capabilities(json["body"]);
        } else {
            ::create();
            body = Capabilities();

        }
        command = "initialize";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "body" : body,
        ]);

        return json;
    }
}

// Information about the capabilities of a debug adapter.
class Capabilities {
    inherit JsonEncodable;

    // json: "additionalModuleColumns"
    array(ColumnDescriptor) additional_module_columns = ({});
    // json: "exceptionBreakpointFilters"
    array(ExceptionBreakpointsFilter) exception_breakpoint_filters = ({});
    // json: "supportedChecksumAlgorithms"
    array(string) supported_checksum_algorithms = ({});
    int supports_completions_request;         // json: "supportsCompletionsRequest"
    int supports_conditional_breakpoints;     // json: "supportsConditionalBreakpoints"
    int supports_configuration_done_request;  // json: "supportsConfigurationDoneRequest"
    int supports_delayed_stack_trace_loading; // json: "supportsDelayedStackTraceLoading"
    int supports_evaluate_for_hovers;         // json: "supportsEvaluateForHovers"
    int supports_exception_info_request;      // json: "supportsExceptionInfoRequest"
    int supports_exception_options;           // json: "supportsExceptionOptions"
    int supports_function_breakpoints;        // json: "supportsFunctionBreakpoints"
    int supports_goto_targets_request;        // json: "supportsGotoTargetsRequest"
    int supports_hit_conditional_breakpoints; // json: "supportsHitConditionalBreakpoints"
    int supports_loaded_sources_request;      // json: "supportsLoadedSourcesRequest"
    int supports_log_points;                  // json: "supportsLogPoints"
    int supports_modules_request;             // json: "supportsModulesRequest"
    int supports_restart_frame;               // json: "supportsRestartFrame"
    int supports_restart_request;             // json: "supportsRestartRequest"
    int supports_set_expression;              // json: "supportsSetExpression"
    int supports_set_variable;                // json: "supportsSetVariable"
    int supports_step_back;                   // json: "supportsStepBack"
    int supports_step_in_targets_request;     // json: "supportsStepInTargetsRequest"
    int supports_terminate_request;           // json: "supportsTerminateRequest"
    int supports_terminate_threads_request;   // json: "supportsTerminateThreadsRequest"
    int supports_value_formatting_options;    // json: "supportsValueFormattingOptions"
    int supports_terminate_debuggee;           // json: "supportTerminateDebuggee"

    protected void create(mixed|void json) {
        if (!json) return;

        additional_module_columns = map(json["additionalModuleColumns"], ColumnDescriptor);
        exception_breakpoint_filters = map(json["exceptionBreakpointFilters"], ExceptionBreakpointsFilter);
        supported_checksum_algorithms = json["supportedChecksumAlgorithms"];
        supports_completions_request = json["supportsCompletionsRequest"];
        supports_conditional_breakpoints = json["supportsConditionalBreakpoints"];
        supports_configuration_done_request = json["supportsConfigurationDoneRequest"];
        supports_delayed_stack_trace_loading = json["supportsDelayedStackTraceLoading"];
        supports_evaluate_for_hovers = json["supportsEvaluateForHovers"];
        supports_exception_info_request = json["supportsExceptionInfoRequest"];
        supports_exception_options = json["supportsExceptionOptions"];
        supports_function_breakpoints = json["supportsFunctionBreakpoints"];
        supports_goto_targets_request = json["supportsGotoTargetsRequest"];
        supports_hit_conditional_breakpoints = json["supportsHitConditionalBreakpoints"];
        supports_loaded_sources_request = json["supportsLoadedSourcesRequest"];
        supports_log_points = json["supportsLogPoints"];
        supports_modules_request = json["supportsModulesRequest"];
        supports_restart_frame = json["supportsRestartFrame"];
        supports_restart_request = json["supportsRestartRequest"];
        supports_set_expression = json["supportsSetExpression"];
        supports_set_variable = json["supportsSetVariable"];
        supports_step_back = json["supportsStepBack"];
        supports_step_in_targets_request = json["supportsStepInTargetsRequest"];
        supports_terminate_request = json["supportsTerminateRequest"];
        supports_terminate_threads_request = json["supportsTerminateThreadsRequest"];
        supports_value_formatting_options = json["supportsValueFormattingOptions"];
        supports_terminate_debuggee = json["supportsTerminateDebuggee"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            // "additionalModuleColumns" : additional_module_columns,
            // "exceptionBreakpointFilters" : exception_breakpoint_filters,
            // "supportedChecksumAlgorithms" : supported_checksum_algorithms,
            // "supportsCompletionsRequest" : supports_completions_request,
            // "supportsConditionalBreakpoints" : supports_conditional_breakpoints,
            "supportsConfigurationDoneRequest" : supports_configuration_done_request,
            // "supportsDelayedStackTraceLoading" : supports_delayed_stack_trace_loading,
            // "supportsEvaluateForHovers" : supports_evaluate_for_hovers,
            // "supportsExceptionInfoRequest" : supports_exception_info_request,
            // "supportsExceptionOptions" : supports_exception_options,
            // "supportsFunctionBreakpoints" : supports_function_breakpoints,
            // "supportsGotoTargetsRequest" : supports_goto_targets_request,
            // "supportsHitConditionalBreakpoints" : supports_hit_conditional_breakpoints,
            // "supportsLoadedSourcesRequest" : supports_loaded_sources_request,
            // "supportsLogPoints" : supports_log_points,
            // "supportsModulesRequest" : supports_modules_request,
            // "supportsRestartFrame" : supports_restart_frame,
            // "supportsRestartRequest" : supports_restart_request,
            // "supportsSetExpression" : supports_set_expression,
             "supportsSetVariable" : supports_set_variable,
            // "supportsStepBack" : supports_step_back,
            // "supportsStepInTargetsRequest" : supports_step_in_targets_request,
            // "supportsTerminateRequest" : supports_terminate_request,
            // "supportsTerminateThreadsRequest" : supports_terminate_threads_request,
            // "supportsValueFormattingOptions" : supports_value_formatting_options,
            // "supportsTerminateDebuggee" : supports_terminate_debuggee,
        ]);
    }
}

// A ColumnDescriptor specifies what module attribute to show in a column of the
// ModulesView, how to format it, and what the column's label should be.
// It is only used if the underlying UI actually supports this level of customization.
class ColumnDescriptor {
    inherit JsonEncodable;

    string attribute_name; // json: "attributeName"
    int  format;         // json: "format"
    string label;          // json: "label"
    string type;           // json: "type"
    int    width;          // json: "width"

    protected void create(mixed|void json) {
        if (!json) return;

        attribute_name = json["attributeName"];
        format = json["format"];
        label = json["label"];
        type = json["type"];
        width = json["width"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "attributeName" : attribute_name,
            "format" : format,
            "label" : label,
            "type" : type,
            "width" : width,
        ]);
    }
}

// An ExceptionBreakpointsFilter is shown in the UI as an option for configuring how
// exceptions are dealt with.
class ExceptionBreakpointsFilter {
    inherit JsonEncodable;

    int    is_default; // json: "default"
    string filter;     // json: "filter"
    string label;      // json: "label"

    protected void create(mixed|void json) {
        if (!json) return;

        is_default = json["default"];
        filter = json["filter"];
        label = json["label"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "default" : is_default,
            "filter" : filter,
            "label" : label,
        ]);
    }
}

// The attach request is sent from the client to the debug adapter to attach to a debuggee
// that is already running. Since attaching is debugger/runtime specific, the arguments for
// this request are not part of this specification.
class AttachRequest {
    inherit Request;

    mixed arguments; // these are implementation-specific. json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = json["arguments"];
        }
        else ::create();

        command = "attach";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Response to 'attach' request. This is just an acknowledgement, so no body field is
// required.
class AttachResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) ::create(json);
        else ::create();

        command = "attach";
    }
}

// The request starts the debuggee to run again.
class ContinueRequest {
    inherit Request;

    ContinueArguments arguments; // these are implementation-specific. json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = json["arguments"];
        }
        else ::create();

        command = "continue";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'continue' request.
class ContinueArguments {
    inherit JsonEncodable;

    int thread_id; // json: threadId

    protected void create(mixed|void json) {
        if (!json) return;

        thread_id = json["threadId"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "threadId" : thread_id,
        ]);
    }
}

// Response to 'continue' request.
class ContinueResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) ::create(json);
        else ::create();

        command = "continue";
    }
}

// The launch request is sent from the client to the debug adapter to start the debuggee
// with or without debugging (if 'noDebug' is true). Since launching is debugger/runtime
// specific, the arguments for this request are not part of this specification.
class LaunchRequest {
    inherit Request;

    mixed arguments; // these are implementation-specific. json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = json["arguments"];
        }
        else ::create();

        command = "launch";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Response to 'launch' request. This is just an acknowledgement, so no body field is
// required.
class LaunchResponse {
        inherit Response;

    protected void create(mixed|void json) {
        if (json) ::create(json);
        else ::create();

        command = "launch";
    }
}
// Evaluates the given expression in the context of the top most stack frame.
// The expression has access to any variables and arguments that are in scope.
class EvaluateRequest {
    inherit Request;

    EvaluateArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = EvaluateArguments(json["arguments"]);
        } else {
            ::create();
            arguments = EvaluateArguments();
        }

        command = "evaluate";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'evaluate' request.
class EvaluateArguments {
    inherit JsonEncodable;

    mixed  context;    // json: "context"
    string expression; // json: "expression"
    mixed  format;     // json: "format"
    mixed  frame_id;   // json: "frameId"

    protected void create(mixed|void json) {
        if (!json) return;

        context = json["context"];
        expression = json["expression"];
        format = json["format"];
        frame_id = json["frameId"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "context" : context,
            "expression" : expression,
            "format" : format,
            "frameId" : frame_id,
        ]);
    }
}

// Response to 'evaluate' request.
class EvaluateResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        } else ::create();

        command = "evaluate";
    }
}

// The request returns the variable scopes for a given stackframe ID.
class ScopesRequest {
    inherit Request;

    ScopesArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = ScopesArguments(json["arguments"]);
        } else {
            ::create();
            arguments = ScopesArguments();
        }

        command = "scopes";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'scopes' request.
class ScopesArguments {
    inherit JsonEncodable;

    int frame_id;

    protected void create(mixed|void json) {
        if (!json) return;

        frame_id = json["frameId"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "frameId" : frame_id
        ]);
    }
}

// Response to 'scopes' request.
class ScopesResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        }
        else {
            ::create();
            body = ([ "scopes" : 0 ]);
        }

        command = "scopes";
    }
}

// Set the variable with the given name in the variable container to a new value.
class SetVariableRequest {
    inherit Request;

    SetVariableArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = SetVariableArguments(json["arguments"]);
        } else {
            ::create();
            arguments = SetVariableArguments();
        }

        command = "setVariable";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'setVariable' request.
class SetVariableArguments {
    inherit JsonEncodable;

    ValueFormat format;              // json: "format"
    string      name;                // json: "name"
    string      value;               // json: "value"
    int         variables_reference; // json: "variablesReference"

    protected void create(mixed|void json) {
        if (!json) return;

        format = json["format"];
        name = json["name"];
        value = json["value"];
        variables_reference = json["variablesReference"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "format" : format,
            "name" : name,
            "value" : value,
            "variablesReference" : variables_reference
        ]);
    }
}

// Response to 'setVariable' request.
class SetVariableResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        }
        else {
            ::create();
            body = ([ "value" : 0 ]);
        }

        command = "setVariable";
    }
}

// The request returns a stacktrace from the current execution state.
class StackTraceRequest {
    inherit Request;

    StackTraceArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = StackTraceArguments(json["arguments"]);
        } else {
            ::create();
            arguments = StackTraceArguments();
        }

        command = "stackTrace";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);

        return json;
    }
}

// Arguments for 'stackTrace' request.
class StackTraceArguments {
    inherit JsonEncodable;

    StackFrameFormat    format;      // json: "format"
    int                 levels;      // json: "levels"
    int                 start_frame; // json: "startFrame"
    int                 thread_id;   // json: "threadId"

    protected void create(mixed|void json) {
        if (!json) return;

        format = StackFrameFormat(json["format"]);
        levels = json["levels"];
        start_frame = json["startFrame"];
        thread_id = json["threadId"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "format" : format,
            "levels" : levels,
            "startFrame" : start_frame,
            "threadId" : thread_id,
        ]);
    }
}

// Response to 'stackTrace' request.
class StackTraceResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        }
        else ::create();

        command = "stackTrace";
    }
}

// The request retrieves a list of all threads.
class ThreadsRequest {
    inherit Request;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
        } else {
            ::create();
        }

        command = "threads";
    }
}

// Response to 'threads' request.
class ThreadsResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        }
        else ::create();

        command = "threads";
    }
}

// Retrieves all child variables for the given variable reference.
// An optional filter can be used to limit the fetched children to either named or indexed
// children.
class VariablesRequest {
    inherit Request;

    VariablesArguments arguments; // json: "arguments"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            arguments = VariablesArguments(json["arguments"]);
        } else {
            ::create();
            arguments = VariablesArguments();
        }

        command = "variables";
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "arguments" : arguments,
        ]);
    }
}

// Arguments for 'variables' request.
class VariablesArguments {
    inherit JsonEncodable;

    int         count;               // json: "count"
    string      filter;              // json: "filter"
    ValueFormat format;              // json: "format"
    int         start;               // json: "start"
    int         variables_reference; // json: "variablesReference"

    protected void create(mixed|void json) {
        if (!json) return;

        count = json["count"];
        filter = json["filter"];
        format = ValueFormat(json["format"]);
        start = json["start"];
        variables_reference = json["variablesReference"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "count" : count,
            "filter" : filter,
            "format" : format,
            "start" : start,
            "variablesReference" : variables_reference,
        ]);
    }
}

// Response to 'variables' request.
class VariablesResponse {
    inherit Response;

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            body = json["body"];
        }
        else ::create();

        command = "variables";
    }
}

// Information about a Breakpoint created in setBreakpoints or setFunctionBreakpoints.
class Breakpoint {
    inherit JsonEncodable;

    int    column;     // json: "column"
    int    end_column; // json: "endColumn"
    int    end_line;   // json: "endLine"
    int    id;         // json: "id"
    int    line;       // json: "line"
    string message;    // json: "message"
    Source source;     // json: "source"
    bool   verified;   // json: "verified"

    protected void create(mixed|void json) {
        if (!json) return;

        column = json["column"];
        end_column = json["endColumn"];
        end_line = json["endLine"];
        id = json["id"];
        line = json["line"];
        message = json["message"];
        source = Source(json["source"]);
        verified = json["verified"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "column" : column,
            "endColumn" : end_column,
            "endLine" : end_line,
            "id" : id,
            "line" : line,
            "message" : message,
            "source" : source,
            "verified" : verified,
        ]);
    }
}

// The checksum of an item calculated by the specified algorithm.
class Checksum {
    inherit JsonEncodable;

    string algorithm; // json: "algorithm"
    string checksum;  // json: "checksum"

    protected void create(mixed|void json) {
        if (!json) return;

        algorithm = json["algorithm"];
        checksum = json["checksum"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "algorithm" : algorithm,
            "checksum" : checksum,
        ]);
    }
}

// A Thread.
class DAPThread {
    inherit JsonEncodable;

    int    id;   // json: "id"
    string name; // json: "name"

    protected void create(mixed|void json) {
        if (!json) return;

        id = json["id"];
        name = json["name"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "id" : id,
            "name" : name
        ]);
    }
}

// The new, changed, or removed module. In case of 'removed' only the module id is used.
//
// A Module object represents a row in the modules view.
// Two attributes are mandatory: an id identifies a module in the modules view and is used
// in a ModuleEvent for identifying a module for adding, updating or deleting.
// The name is used to minimally render the module in the UI.
//
// Additional attributes can be added to the module. They will show up in the module View if
// they have a corresponding ColumnDescriptor.
//
// To avoid an unnecessary proliferation of additional attributes with similar semantics but
// different names
// we recommend to re-use attributes from the 'recommended' list below first, and only
// introduce new attributes if nothing appropriate could be found.
class Module {
    inherit JsonEncodable;

    string address_range;    // json: "addressRange"
    string date_time_stamp;  // json: "dateTimeStamp"
    int|string  id;          // json: "id"
    bool   is_optimized;     // json: "isOptimized"
    bool   is_user_code;     // json: "isUserCode"
    string name;             // json: "name"
    string path;             // json: "path"
    string symbol_file_path; // json: "symbolFilePath"
    string symbol_status;    // json: "symbolStatus"
    string version;          // json: "version"


    protected void create(mixed|void json) {
        if (!json) return;

        address_range = json["addressRange"];
        date_time_stamp = json["dateTimeStamp"];
        id = json["id"];
        is_optimized = json["isOptimized"];
        is_user_code = json["isUserCode"];
        name = json["name"];
        path = json["path"];
        symbol_file_path = json["symbolFilePath"];
        symbol_status = json["symbolStatus"];
        version = json["version"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "addressRange" : address_range,
            "dateTimeStamp" : date_time_stamp,
            "id" : id,
            "isOptimized" : is_optimized,
            "isUserCode" : is_user_code,
            "name" : name,
            "path" : path,
            "symbolFilePath" : symbol_file_path,
            "symbolStatus" : symbol_status,
            "version" : version,
        ]);
    }
}

// A Scope is a named container for variables. Optionally a scope can map to a source or a
// range within a source.
class Scope {
    int     column;              // json: "column"
    int     end_column;          // json: "endColumn"
    int     end_line;            // json: "endLine"
    bool    expensive;           // json: "expensive"
    int     indexed_variables;   // json: "indexedVariables"
    int     line;                // json: "line"
    string  name;                // json: "name"
    int     named_variables;     // json: "namedVariables"
    Source  source;              // json: "source"
    int     variables_reference; // json: "variablesReference"

    protected void create(mixed|void json) {
        if (!json) return;

        column = json["column"];
        end_column = json["endColumn"];
        end_line = json["endLine"];
        expensive = json["expensive"];
        indexed_variables = json["indexedVariables"];
        line = json["line"];
        name = json["name"];
        named_variables = json["namedVariables"];
        source = Source(json["source"]);
        variables_reference = json["variablesReference"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "column" : column,
            "endColumn" : end_column,
            "endLine" : end_line,
            "expensive" : expensive,
            "indexedVariables" : indexed_variables,
            "line" : line,
            "name" : name,
            "namedVariables" : named_variables,
            "source" : source,
            "variablesReference" : variables_reference,
        ]);
    }
}

// A Source is a descriptor for source code. It is returned from the debug adapter as part
// of a StackFrame and it is used by clients when specifying breakpoints.
class Source {
    inherit JsonEncodable;

    mixed           adapter_data;      // json: "adapterData"
    array(Checksum) checksums;         // json: "checksums"
    string          name;              // json: "name"
    string          origin;            // json: "origin"
    string          path;              // json: "path"
    string          presentation_hint; // json: "presentationHint"
    int             source_reference;  // json: "sourceReference"
    array(Source)   sources;           // json: "sources"

    protected void create(mixed|void json) {
        if (!json) return;

        adapter_data = json["adapterData"];
        checksums = map(json["checksums"], Checksum);
        name = json["name"];
        origin = json["origin"];
        path = json["path"];
        presentation_hint = json["presentationHint"];
        source_reference = json["sourceReference"];
        sources = map(json["sources"], Source);
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "adapterData" : adapter_data,
            "checksums" : checksums,
            "name" : name,
            "origin" : origin,
            "path" : path,
            "presentationHint" : presentation_hint,
            "sourceReference" : source_reference,
            "sources" : sources,
        ]);
    }
}

// Specifies how the resulting value should be formatted.
class ValueFormat {
    inherit JsonEncodable;

    bool hex; // json: "hex"

    protected void create(mixed|void json) {
        if (!json) return;

        hex = json["hex"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "hex" : hex,
        ]);
    }
}

// Provides formatting information for a stack frame.
class StackFrameFormat {
    inherit ValueFormat;

    bool include_all;      // json: "includeAll"
    bool line;             // json: "line"
    bool module;           // json: "module"
    bool parameter_names;  // json: "parameterNames"
    bool parameters;       // json: "parameters"
    bool parameter_types;  // json: "parameterTypes"
    bool parameter_values; // json: "parameterValues"

    protected void create(mixed|void json) {
        if (json) {
            ::create(json);
            include_all = json["includeAll"];
            line = json["line"];
            module = json["module"];
            parameter_names = json["parameterNames"];
            parameters = json["parameters"];
            parameter_types = json["parameterTypes"];
            parameter_values = json["parameterValues"];
        } else {
            ::create();
        }
    }

    protected mapping(string:mixed) to_json() {
        mapping(string:mixed) json = ::to_json();
        json += ([
            "includeAll" : include_all,
            "line" : line,
            "module" : module,
            "parameterNames" : parameter_names,
            "parameters" : parameters,
            "parameterTypes" : parameter_types,
            "parameterValues" : parameter_values,
        ]);
    }
}

// A Variable is a name/value pair.
// Optionally a variable can have a 'type' that is shown if space permits or when hovering
// over the variable's name.
// An optional 'kind' is used to render additional properties of the variable, e.g.
// different icons can be used to indicate that a variable is public or private.
// If the value is structured (has children), a handle is provided to retrieve the children
// with the VariablesRequest.
// If the number of named or indexed children is large, the numbers should be returned via
// the optional 'namedVariables' and 'indexedVariables' attributes.
// The client can use this optional information to present the children in a paged UI and
// fetch them in chunks.
class Variable {
    inherit JsonEncodable;

    string                      evaluate_name;       // json: "evaluateName"
    int                         indexed_variables;   // json: "indexedVariables"
    string                      name;                // json: "name"
    int                         named_variables;     // json: "namedVariables"
    VariablePresentationHint    presentation_hint;   // json: "presentationHint"
    string                      type;                // json: "type"
    string                      value;               // json: "value"
    int                         variables_reference; // json: "variablesReference"

    protected void create(mixed|void json) {
        if (!json) return;

        evaluate_name = json["evaluateName"];
        indexed_variables = json["indexedVariables"];
        name = json["name"];
        named_variables = json["namedVariables"];
        presentation_hint = VariablePresentationHint(json["presentationHint"]);
        type = json["type"];
        value = json["value"];
        variables_reference = json["variablesReference"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "evaluateName" : evaluate_name,
            "indexedVariables" : indexed_variables,
            "name" : name,
            "namedVariables" : named_variables,
            "presentationHint" : presentation_hint,
            "type" : type,
            "value" : value,
            "variablesReference" : variables_reference,
        ]);
    }
}

// Properties of a value that can be used to determine how to render the result in the UI.
class VariablePresentationHint {
    inherit JsonEncodable;

    array(string)   attributes; // json: "attributes"
    string          kind;       // json: "kind"
    string          visibility; // json: "visibility"

    protected void create(mixed|void json) {
        if (!json) return;

        attributes = json["attributes"];
        kind = json["kind"];
        visibility = json["visibility"];
    }

    protected mapping(string:mixed) to_json() {
        return
        ([
            "attributes" : attributes,
            "kind" : kind,
            "visibility" : visibility,
        ]);
    }
}