void fail()
{ 
// can't connect to socket - this is what we expect
   exit(0); // ok
}

void ok()
{
   if (f->write("hej")==-1)
      werror("connecttest.pike: succeeded to connect to closed socket"
	     " (port %d)\n",z);
   else
      werror("connecttest.pike: socket still open (??)"
	     " (port %d)\n",z);

   exit(1);      
}

object f=Stdio.File();
int z=16384+random(10000);

int main()
{
   object p=Stdio.Port();
   while (!p->bind(z=(z-1023)%32768+1024));
//     werror("port: %d\n",z);
   destruct(p); // this port can't be connected to now
   sleep(0.1);
   
   f->open_socket();
   f->set_nonblocking(0,ok,fail);
   f->connect("localhost",z);
   return -1;
}

