//
// LPD.pmod: an implementation of the BSD lpd protocol (RFC 1179).
// This is a module for pike.
// 3 July 1998 <hww3@riverweb.com> Bill Welliver
//
// $Id: LPD.pmod,v 1.4 1999/08/20 05:01:58 hubbe Exp $
//

class client {
  string host;
  int port;
  object conn;
  int jobnum;
  string jobtype;
  string jobname;

  int set_job_type(string type)
  {
    type=lower_case(type);
    
    switch (type) { 
     case "f":
     case "text":
      jobtype="f";
      break;

     case "o":
     case "postscript":
     case "ps":
      jobtype="o";
      break;

     default:
     case "l":
     case "raw":
      jobtype="l";
      break;
    }
    return 1;
  }

  int set_job_name(string name)
  {
    jobname=name;
    return 1;
  }

  string|int start_queue(string queue)
  {
    if(!queue) return 0;

    if(!conn->connect(host, port))
      return 0;

    conn->write(sprintf("%c%s\n", 01, queue));
    string resp= conn->read();
    conn->close();
    return 1;
  }

  string|int send_job(string queue, string job)
  {
    int r;
    string resp;

    if(!queue) return 0;

    if(!conn->connect(host, port))
      return 0;
    werror("connected to " + host + "\n");

    conn->write(sprintf("%c%s\n", 02, queue));
    // werror("sent send data command\n");
    string control="";
    control+="H"+gethostname()+"\n";
#if constant(getuid) && constant(getpwuid)
    control+="P"+(getpwuid(getuid())[0]||"nobody")+"\n";
#else
    /* Should there be a P-1 here? /Hubbe */
#endif
    control+=(jobtype||"l")+"dfA"+ sprintf("%3d%s", jobnum, gethostname())+"\n";
    if(jobname)
      control+="J" + jobname + "\n";
    jobnum++;
    conn->write(sprintf("%c%s cfA%3d%s\n", 02, (string)sizeof(control),
			jobnum,gethostname()));
    // werror("getting ready to send control file\n");

    conn->write(sprintf("%s%c", control, 0));
    // werror("sent control file\n");

    conn->write(sprintf("%c%s dfA%3d%s\n", 03, (string)sizeof(job), jobnum,
			gethostname()));
    conn->write(sprintf("%s%c", job, 0));
    // werror("sent data file\n");

    conn->close();
    start_queue(queue);

    return 1;
  }

  string|int delete_job(string queue, int|void job)
  {
    if(!queue) return 0;

    if(!conn->connect(host, port))
      return 0;

#if constant(getpwuid) && constant(getuid)
    string agent=(getpwuid(getuid())[0]||"nobody");
#else
    string agent="nobody";
#endif

    if(job)
      conn->write(sprintf("%c%s %s %d\n", 05, queue, agent, job));
    else
      conn->write(sprintf("%c%s %s\n", 05, queue, agent));
    string resp= conn->read();
    conn->close();
    return 1;
  }


  string|int status(string queue)
  {
    if(!queue) return 0;

    if(!conn->connect(host, port))
      return 0;

    conn->write(sprintf("%c%s\n", 04, queue));
    string resp= conn->read();
    conn->close();
    return resp;
  }

  void create(string|void hostname, int|void portnum)
  {
    host=hostname || "localhost";
    port=portnum || 515;
    conn=Stdio.File();
    jobnum=0;
  }
}

