#ifndef TEST_NORMAL

#define PRE "connecttest (closed): "

void fail()
{ 
// can't connect to socket - this is what we expect
   werror(PRE"everything ok\n");
   exit(0); // ok
}

void ok()
{
   if (f->write("hej")==-1)
      werror(PRE"succeeded to connect to closed socket"
	     " (port %d)\n",z);
   else
      werror(PRE"socket still open (??)"
	     " (port %d)\n",z);

   exit(1);      
}

#else

#define PRE "connecttest (open): "

void fail()
{ 
// can't connect to socket - this is what we expect
   werror(PRE"can't connect to open port; failure reported\n");
   exit(1); // ok
}

void ok()
{
   if (f->write("hej")==-1)
   {
      werror(PRE"connected ok, but socket closed"
	     " (port %d)\n",z);
      exit (1);
   }
   else
   {
      werror(PRE"everything ok"
	     " (port %d)\n",z);
      exit (0);
   }
}

#endif

void rcb(){}

void timeout()
{
   werror(PRE"timeout - connection neither succeded "
	  "nor failed\n");
   exit(1);
}

object f=Stdio.File();
int z=16384+random(10000);
object p=Stdio.Port();

int main()
{
   while (!p->bind(z=(z-1023)%32768+1024));
//     werror("port: %d\n",z);
#ifndef TEST_NORMAL
   destruct(p); // this port can't be connected to now
#endif

   werror(PRE"using port %d\n",z);

   sleep(0.1);
   
   f->open_socket();
   f->set_nonblocking(rcb,ok,fail);
   if (catch { f->connect("127.0.0.1",z); } &&
       catch { f->connect("localhost",z); })
   {
      werror(PRE"failed to connect "
	     "to \"localhost\" nor \"127.0.0.1\"\n");
      werror("reporting ok\n");
      return 0;
   }
   call_out(timeout,1);
   return -1;
}

