
protected string id;
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
  if(sizeof(id) == 24)
    sscanf(id, "%8x%6x%4x%6x", timestamp, host, pid, counter);
  else if(sizeof(id) == 12)
    sscanf(id, "%4c%3c%2c%3c", timestamp, host, pid, counter);
  else throw(Error.Generic("Invalid ObjectId format.\n"));
}

string get_id()
{ 
  return sprintf("%4c%3c%2c%3c", timestamp, host, pid, counter);
}

string _sprintf(mixed t)
{
	return "ObjectId(" + String.string2hex(get_id()) + ")";
}

static mixed cast(string t)
{
  if(t == "string")
    return String.string2hex(get_id());
  else
    throw(Error.Generic("invalid cast of ObjectId to " + t + "\n"));
}
