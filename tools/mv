#!/usr/local/bin/pike

inherit "lib.pike";

int main(int argc, array(string) argv)
{
  if(sscanf(argv[-1],"%*[a-zA-Z]:%*s")==2)
  {
    argv[0]="rename";
    if(argv[1]=="-c") argv=argv[..0]+argv[2..];
    exit(do_cmd( Array.map(argv,fixpath)));
  }else{
    argv[0]=find_next_in_path(argv[0],"mv");
    if(file_stat(argv[1]+".exe"))
    {
       array(string) cmd=({argv[0],argv[1]+".exe",argv[-1]});
       mixed s=file_stat(cmd[-1]);
       if(!s || s[1]!=-2) cmd[-1]+=".exe";

       int ret=Process.create_process(cmd)->wait();
       if(ret) exit(ret);
    }
    exece(argv[0],argv[1..]);
    exit(69);
  }
}
