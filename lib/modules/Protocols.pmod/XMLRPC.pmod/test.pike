constant XMLRPC = .module;

constant params =
({
  4711,
  "Poderoso caballero es don dinero.",
  7.0,
  ([
    "a":76,
    "b":77,
    "b":81,
    "c":92,
    "d":({ 8.0, "Samba Pa Ti" })
  ]),
  ({ 1, 2, 3 })
});

constant fault_string = "This is a bogus error.";

int main(int argc, array(string) argv)
{
  XMLRPC.Call call = XMLRPC.decode_call(XMLRPC.encode_call("f", params));
  if(!equal(call->params, params))
    error("XMLRPC.decode_call and XMLRPC.encode_call params mismatch.\n");
  if(!equal(call->method_name, "f"))
    error("XMLRPC.decode_call and XMLRPC.encode_call method name mismatch.\n");
  
  array response = XMLRPC.decode_response(XMLRPC.encode_response(params));
  if(!equal(response, params))
    error("XMLRPC.decode_call and XMLRPC.encode_call response mismatch.\n");
  
  XMLRPC.Fault fault = XMLRPC.decode_response(XMLRPC.encode_response_fault
					      (42, fault_string));
  if(fault->fault_code != 42 || fault->fault_string != fault_string)
    error("XMLRPC.decode_call and XMLRPC.encode_call fault mismatch.\n");
  
  return 0;
}
