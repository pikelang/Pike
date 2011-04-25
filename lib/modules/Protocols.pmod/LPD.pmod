//
// LPD.pmod: an implementation of the BSD lpd protocol (RFC 1179).
// This is a module for pike.
// 3 July 1998 <hww3@riverweb.com> Bill Welliver
//
// $Id$
//

#pike __REAL_VERSION__

//! A client for communicating with printers and print spoolers that
//! support the BSD lpd protocol (RFC 1179).
class client {
  string host;
  int port;
  private object conn;
  int jobnum;
  string jobtype;
  string jobname;

  private int connect(string host, int port)
  {
    int a=random(10);
    // try to open one of the "official" local socket ports.
    // not having one doesn't seem to be a problem with most LPD 
    // servers, but we should at least try. will probably fail
    // if two try to open the same local port at once. ymmv.
    int res=conn->open_socket(721 + a);

    return conn->connect(host, port);
  }

//! @decl int set_job_type(string type)
//! Set the type of job to be sent to the printer to @i{type@}.
//! Valid types are: text, postscript and raw.
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

//! @decl int set_job_name(string name)
//! Sets the name of the print job to @i{name@}.
  int set_job_name(string name)
  {
    jobname=name;
    return 1;
  }

//! @decl int start_queue(string queue)
//! Start the queue @i{queue@} if not already printing.
//! @returns
//! Returns 0 if unable to connect, 1 otherwise. 
  int start_queue(string queue)
  {
    if(!queue) return 0;

    if(!connect(host, port))
      return 0;

    conn->write(sprintf("%c%s\n", 01, queue));
    string resp= conn->read();
    conn->close();
    return 1;
  }

//! @decl string|int send_job(string queue, string job)
//! Send print job consisting of data @i{job@} to printer @i{queue@}.
//! @returns
//! Returns 1 if success, 0 otherwise.
  int send_job(string queue, string job)
  {
    int r;
    string resp;

    if(!queue) return 0;

    if(!connect(host, port))
      return 0;
    // werror("connected to " + host + "\n");

    string control="";
    control+="H"+gethostname()+"\n";
#if constant(getuid) && constant(getpwuid)
    control+="P"+(getpwuid(getuid())[0]||"nobody")+"\n";
#else
    control+="P-1\n";
#endif
    control+=(jobtype||"l")+"dfA"+ sprintf("%03d%s", jobnum, gethostname())+"\n";
    if(jobname)
    {
      control+="J" + jobname + "\n";
      control+="N" + jobname + "\n";
    }
    else
    { 
      control+="JPike LPD Client Job " + jobnum + "\n";
      control+="NPike LPD Client Job " + jobnum + "\n";
    }
    jobnum++;
werror("job file:\n\n" + control  + "\n\n");

    conn->write(sprintf("%c%s\n", 02, queue));
    resp=conn->read(1);
    if((int)resp !=0)
    {
      werror("receive job failed.\n");
      return 0;
    }

    conn->write(sprintf("%c%s cfA%03d%s\n", 02, (string)sizeof(control),
			jobnum,gethostname()));

    resp=conn->read(1);
    if((int)resp !=0)
    {
      werror("request receive control failed.\n");
      return 0;
    }

    conn->write(sprintf("%s%c", control, 0));

    resp=conn->read(1);
    if((int)resp !=0)
    {
      werror("send receive control failed.\n");
      return 0;
    }

    conn->write(sprintf("%c%s dfA%03d%s\n", 03, (string)sizeof(job), jobnum,
			gethostname()));
    resp=conn->read(1);
    if((int)resp !=0)
    {
      werror("request receive job failed.\n");
      return 0;
    }


    conn->write(sprintf("%s%c", job, 0));

    resp=conn->read(1);
    if((int)resp !=0)
    {
      werror("send receive job failed.\n");
      return 0;
    }



    // read the response. 

//    resp=conn->read();
    if((int)(resp)!=0) 
    { 
      conn->close();
      return 0;
    }
    conn->close();
//    start_queue(queue);
    return 1;
  }

//! @decl int delete_job(string queue, int|void job)
//! Delete job @i{job@} from printer @i{queue@}.
//! @returns
//! Returns 1 on success, 0 otherwise.
  int delete_job(string queue, int|void job)
  {
    if(!queue) return 0;

    if(!connect(host, port))
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


//! @decl string|int status(string queue)
//! Check the status of queue @i{queue@}.
//! @returns
//! Returns 0 on failure, otherwise returns the status response from the printer.
  string|int status(string queue)
  {
    if(!queue) return 0;

    if(!connect(host, port))
      return 0;

    conn->write(sprintf("%c%s\n", 04, queue));
    string resp= conn->read();
    conn->close();
    return resp;
  }

//! Create a new LPD client connection.
//! @param hostname
//! Contains the hostname or ipaddress of the print host.
//! if not provided, defaults to @i{localhost@}.
//! @param portnum
//! Contains the port the print host is listening on.
//! if not provided, defaults to port @i{515@}, the RFC 1179 standard.
  void create(string|void hostname, int|void portnum)
  {
    host=hostname || "localhost";
    port=portnum || 515;
    conn=Stdio.File();
    jobnum=1;
  }
}

