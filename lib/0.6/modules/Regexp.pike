// $Id: Regexp.pike,v 1.2 2007/10/12 15:48:06 grubba Exp $

#pike 7.0

inherit Regexp;

// Hide replace().
private string replace(string in, string|function(string:string) transform)
{
  return ::replace(in, transform);
  replace;	// Disable warning for unused symbol...
}

static string _sprintf()
{
  return "Regexp 0.6";
}
