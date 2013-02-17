
string id;
int timestamp;
int host;
int pid;
int counter;

//!
static void create(string|void _id)
{
  if(_id)
  {
    id = _id;
    populate();
  }
  else
    generate_id();
}

static void generate_id()
{
    id = sprintf("%4c%3s%2c%3c", time(), make_host_hash(), getpid(), make_counter());
    populate();
}

static string make_host_hash()
{
  return Crypto.MD5()->update(System.gethostname())->digest(3);
}

static int make_counter()
{
  return .getCounter();
}

static void populate()
{
  sscanf(id, "%4c%3c%2c%3c", timestamp, host, pid, counter);
}

string get_id()
{ 
  return id;
}

string _sprintf(mixed t)
{
	return "ObjectId(" + String.string2hex(id) + ")";
}
