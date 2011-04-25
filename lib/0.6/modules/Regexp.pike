// $Id$

#pike 7.0

inherit Regexp;

// Hide replace().
private string replace(string in, string|function(string:string) transform)
{
  return ::replace(in, transform);
}

static string _sprintf()
{
  return "Regexp 0.6";
}
