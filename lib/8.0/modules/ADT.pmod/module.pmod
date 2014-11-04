#pike 8.1

inherit ADT : pre;

class struct
{
  inherit pre::struct;

  string `buffer()
  {
    return contents();
  }

  int `index()
  {
    return 0;
  }
}