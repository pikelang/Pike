class protocol
{
  // Maybe this should be the other way around?
  inherit NNTP.protocol;
}

class client
{
  inherit protocol;

  void create(void|string server)
  {
    if(!server)
    {
      // Lookup MX record here (Using DNS.pmod)
      object dns=master()->resolv("Protocols")["DNS"]->client();
      server=dns->get_primary_mx(gethostname());
    }

    if(!connect(server,25))
    {
      throw(({"Failed to connect to news server.\n",backtrace()}));
    }

    if(readreturncode()/100 != 2)
      throw(({"Connection refused by SMTP server.\n",backtrace()}));

    if(command("EHLO "+gethostname())/100 !=2)
      if(command("HELO "+gethostname())/100 != 2)
	throw(({"SMTP: greeting failed.\n",backtrace()}));
  }

  void send_message(string *to, string body)
  {
    // Not yet done
  }
}
