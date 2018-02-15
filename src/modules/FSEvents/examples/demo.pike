
object m;

void cb(string path, int flags, int event_id)
{
  werror("%s => %s, id=%d\n", path, System.FSEvents.describe_event_flag(flags), event_id);
}

int main(int argc, array(string) argv)
{
   string dir = "/tmp";
   if(sizeof(argv))
    dir = argv[1];

#if constant(Pike.DefaultBackend.HAVE_CORE_FOUNDATION)
   Pike.DefaultBackend.enable_core_foundation(1);
#else
   werror("Pike does not have support for Core Foundation. FSEvents will not function!\n");
   exit(1);
#endif

   werror("watching %O for changes...\n", dir);

   m = System.FSEvents.EventStream(({}), 3.0, System.FSEvents.kFSEventStreamEventIdSinceNow, 
    System.FSEvents.kFSEventStreamCreateFlagNone);

   m->add_path(dir);
   m->callback_func = (cb); 
   m->start();

   return -1;
}
