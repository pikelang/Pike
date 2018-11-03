#pike 8.1
#pragma no_deprecation_warnings

inherit ADT.struct;

string `buffer()
{
  return contents();
}

int `index()
{
  return 0;
}
