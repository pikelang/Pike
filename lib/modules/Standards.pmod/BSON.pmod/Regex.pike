#pike __REAL_VERSION__

constant BSONRegex = 1;

string regex;
string options;

//!
protected void create(string regex, void|string options)
{
  this::regex = regex;
  this::options = sort( (options || "")/1 )*"";
}

protected string _sprintf(int t)
{
  return t=='O' && sprintf("%O(%O,%O)", this_program, regex, options);
}
