#pike 8.1

inherit ADT.struct;

string `buffer()
{
  return contents();
}

int `index()
{
  return 0;
}
