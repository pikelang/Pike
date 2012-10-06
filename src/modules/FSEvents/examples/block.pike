
object m;

void cb(string path, int flags, int event_id)
{
  werror("%s => %s, id=%d\n", path, System.FSEvents.describe_event_flag(flags), event_id);
}

int main(int argc, array(string) argv)
{
   string dir = "/tmp";
   if(sizeof(argv) > 1)
    dir = argv[1];

   werror("watching %O for changes...\n", dir);

   m = System.FSEvents.BlockingEventStream(({}), 3.0, System.FSEvents.kFSEventStreamEventIdSinceNow, 
    System.FSEvents.kFSEventStreamCreateFlagNone);

   m->add_path(dir);

   do
   {
     write("%O\n", m->read_event());
  } while(1);
}
