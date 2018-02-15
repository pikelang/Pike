#pike 7.7

inherit Parser.XML.Tree;

// There was no simple_parse_input in 7.2.

Node parse_input (string data, void|int(0..1) no_fallback,
		  void|int(0..1) force_lowercase,
		  void|mapping(string:string) predefined_entities,
		  void|int(0..1) parse_namespaces,
		  ParseFlags|void flags)
{
  return ::parse_input (data, no_fallback, force_lowercase,
			predefined_entities, parse_namespaces,
			flags | PARSE_COMPAT_ALLOW_ERRORS_7_2);
}
