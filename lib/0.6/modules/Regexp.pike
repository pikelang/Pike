// $Id: Regexp.pike,v 1.3 2008/06/28 16:50:22 nilsson Exp $

#pike 7.0

inherit Regexp;

// Hide replace().
private string replace(string in, string|function(string:string) transform)
{
  return ::replace(in, transform);
  replace;	// Disable warning for unused symbol...
}

protected string _sprintf()
{
  return "Regexp 0.6";
}
