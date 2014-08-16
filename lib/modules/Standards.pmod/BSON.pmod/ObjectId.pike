#pike __REAL_VERSION__

protected string id;
int timestamp;
int host;
int pid;
int counter;
private function getCounter;

//!
protected void create(string|void _id)
{
  if(_id)
  {
    id = _id;
    populate();
  }
  else
    generate_id();
}

protected void generate_id()
{
    id = sprintf("%4c%3s%2c%3c", time(), make_host_hash(), getpid(), make_counter());
    populate();
}

protected string make_host_hash()
{
  return Crypto.MD5.hash(System.gethostname())[..2];
}

protected int make_counter()
{
  if(!getCounter) getCounter = master()->resolv("Standards.BSON.getCounter");
  return getCounter();
}

protected void populate()
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

protected string _sprintf(mixed t)
{
  return "ObjectId(" + String.string2hex(get_id()) + ")";
}

protected mixed cast(string t)
{
  if(t == "string")
    return String.string2hex(get_id());
  return UNDEFINED;
}
