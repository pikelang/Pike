/* test.pike
 *
 */

class backend
{
  int login(string user, string passwd)
    {
      werror("login: %s:%s\n", user, passwd);
      return 17;
    }

  array capabilities()
    {
      werror("capability\n");
      return ({ "IMAP4rev1" });
    }

  array update()
    {
      werror("update\n");
      return 0;
    }
}

int main(int argc, array(string) argv)
{
  int port = (argc == 2) ? (int) argv[1] : 143;

  .imap_server(port, backend());

  return -17;
}
  
