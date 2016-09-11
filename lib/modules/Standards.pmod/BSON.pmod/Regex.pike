#pike __REAL_VERSION__

constant BSONRegex = 1;

string regex;
string options;

//!
protected void create(string regex, string options)
{
  this::regex = regex;
  this::options = options;
}

protected string _sprintf(int t)
{
  return t=='O' && sprintf("%O(%O,%O)", this_program, regex, options);
}
