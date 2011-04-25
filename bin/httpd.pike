#!/usr/local/bin/pike

/* $Id$ */

/* A very small httpd capable of fetching files only.
 * Written by Fredrik Hübinette as a demonstration of Pike
 */

#include <simulate.h>

inherit Stdio.Port;

/* number of bytes to read for each write */
#define BLOCK 16060

/* Where do we have the html files ? */
#define BASE "/home/hubbe/pike/src/"

/* File to return when we can't find the file requested */
#define NOFILE "/home/hubbe/www/html/nofile.html"

/* Port to open */
#define PORT 1905

program output_class=class
{
  inherit Stdio.File : socket;
  inherit Stdio.File : file;

  int offset=0;

  void write_callback()
  {
    int written;
    string data;

    file::seek(offset);
    data=file::read(BLOCK);
    if(data && strlen(data))
    {
      written=socket::write(data);
      if(written >= 0)
      {
	offset+=written;
	return;
      }
      perror("Error: "+socket::errno()+".\n");
    }
    destruct(this_object());
  }

  string input="";

  void read_callback(mixed id,string data)
  {
    string cmd;

    input+=data;
    if(sscanf(input,"%s %s%*[\012\015 \t]",cmd,input))
    {
      if(cmd!="GET")
      {
	perror("Only method GET is supported.\n");
	destruct(this_object());
	return;
      }

      sscanf(input,"%*[/]%s",input);
      input=combine_path(BASE,input);
      
      if(!file::open(input,"r"))
      {
	if(!file::open(NOFILE,"r"))
	{
	  perror("Couldn't find default file.\n");
	  destruct(this_object());
	  return;
	}
      }

      socket::set_buffer(65536,"w");
      socket::set_nonblocking(0,write_callback,0);
      write_callback();
    }
  }

  void selfdestruct() { destruct(this_object()); }

  void create(object f)
  {
    socket::assign(f);
    socket::set_nonblocking(read_callback,0,selfdestruct);
  }
};

void accept_callback()
{
  object tmp_output;
  tmp_output=accept();
  if(!tmp_output) return;
  output_class(tmp_output);
  destruct(tmp_output);
}

int main(int argc, string *argv)
{
  perror("Starting minimal httpd\n");

  if(!bind(PORT, accept_callback))
  {
    perror("Failed to open socket (already bound?)\n");
    return 17;
  }

  return - 17; /* Keep going */
}
