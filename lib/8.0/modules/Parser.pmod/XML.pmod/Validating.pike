#pike 8.1

inherit Parser.XML.Validating;

string get_external_entity(string sysid, string|void pubid,
			   mapping|int|void info,
			   mixed ... extra)
{
  // Override this function
  return 0;
}
