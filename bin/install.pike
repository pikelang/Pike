#!/usr/local/bin/pike

#define USE_GTK

#if !constant(GTK.parse_rc)
#undef USE_GTK
#endif

int last_len;
int redump_all;
string pike;
array(string) files_to_delete=({});
array(string) files_to_not_delete=({});
array(string) to_dump=({});
array(string) to_export=({});

int export;
int no_gui;

Tools.Install.ProgressBar progress_bar;

/* for progress bar */
int files_to_install;
int installed_files;

#define MASTER_COOKIE "(#*&)@(*&$Master Cookie:"


int istty_cache;
int istty()
{
#ifdef __NT__
  return 1;
#else
  if(!istty_cache)
  {
    istty_cache=!!Stdio.stdin->tcgetattr();
    if(!istty_cache)
    {
      istty_cache=-1;
    }else{
      switch(getenv("TERM"))
      {
	case "dumb":
	case "emacs":
	  istty_cache=-1;
      }
    }
  }
  return istty_cache>0;
#endif
}

void status1(string fmt, mixed ... args)
{
  status_clear();
#ifdef USE_GTK
  if(label1)
  {
    label7->set_text(sprintf(fmt,@args)-"\n");
    GTK.flush();
    return;
  }
#endif

  // Ugly thing, but status_clear does not indent in non-tty mode...
  if(!istty())
    write("   ");
  
  write("%s\n",sprintf(fmt,@args));
}

string some_strerror(int err)
{
  string ret;
#if constant(strerror)  
  ret=strerror(err);
#endif
  if(!ret || search("unknown error",lower_case(ret))!=-1)
    ret=sprintf("errno=%d",err);

  return ret;
}

void fail(string fmt, mixed ... args)
{
  int err=errno();
#ifdef USE_GTK
  if(label1)
  {
    status1(fmt,@args);
    hbuttonbox1->add(button1=GTK.Button("Exit")->show());
    button1->signal_connect("pressed",do_exit,0);

    label6->set_text("Click Exit to exit installation program.");

    // UGLY!!! -Hubbe
    while(1) { sleep(0.1); GTK.flush(); }
  }
#endif


  if(last_len) write("\n");
  werror("%s: %s\n",sprintf(fmt,@args),some_strerror(err));
  werror("Current directory = %s\n",getcwd());
  werror("**Installation failed..\n");
  exit(1);
}


void status(string doing, void|string file, string|void msg)
{
  if(!file) file="";
#ifdef USE_GTK
  if(label1)
  {
    last_len=1;
    label1->set_text(doing || "");
    label2->set_text(dirname(file) || "");
    label5->set_text(basename(file) || "");
    label6->set_text(msg || "");
    GTK.flush();
    return;
  }
#endif

  if(!istty()) return;

  if(progress_bar)
  {
    last_len = 75;
    progress_bar->set_current(installed_files);
    progress_bar->update(0);
    return;
  }

  file=replace(file,"\n","\\n");
  if(strlen(file)>45)
    file=".."+file[strlen(file)-44..];

  if(msg) file+=" "+msg;
  if(doing) file=doing+": "+file;
  string s="\r   "+file;
  int t=strlen(s);
  if(t<last_len) s+=" "*(last_len-t);
  last_len=t;
  write(s);
}

void status_clear(void|int all)
{
    if(all)
	last_len = 75;
    status(0,"");
    status(0,"");
}


mapping already_created=([]);
int mkdirhier(string orig_dir)
{
  int tomove;
  if(export) return 1;
  string dir=orig_dir;
  if(already_created[orig_dir]) return 1;

  if(dir=="" || (strlen(dir)==2 && dir[-1]==':')) return 1;
  dir=fakeroot(dir);

  status("Creating",dir+"/");

  mixed s=file_stat(dir);
  if(s)
  {
    if(s[1]<0)
      return already_created[orig_dir]=1;

    if(glob("*.pmod",dir))
    {
      if(!mv(dir,dir+".tmp"))
	fail("mv(%s,%s)",dir,dir+".tmp");
      tomove=1;
    }else{
      // FIXME: ask user if he wants to override
      werror("Warning: Directory '%s' already exists as a file.\n",dir);
      if(!mv(dir,dir+".old"))
	fail("mv(%s,%s)",dir,dir+".old");
    }
  }

  mkdirhier(dirname(dir));
  if(!mkdir(dir))
    fail("mkdir(%s)",dir);

  chmod(dir,0755);

  if(tomove)
    if(!mv(dir+".tmp",dir+"/module.pmod"))
      fail("mv(%s,%s)",dir+".tmp",dir+"/module.pmod");

  return already_created[orig_dir]=1;
}

int compare_files(string a,string b)
{
  mixed sa=file_stat(a);
  mixed sb=file_stat(b);
  if(sa && sb && sa[1]==sb[1])
    return Stdio.read_file(a) == Stdio.read_file(b);
  return 0;
}

int low_install_file(string from,
		     string to,
		     void|int mode)
{
  installed_files++;
  if(export)
  {
//    werror("FROM: %O\n",from);
    to_export+=({ from });
    return 1;
  }
  
  to=fakeroot(to);

  status("Installing",to);

  if(compare_files(from,to))
  {
    status("Installing",to,"Already installed");
    return 0;
  }
  mkdirhier(dirname(to));
  switch(query_num_arg())
  {
    case 2:
      mode=0755;
  }
  string tmpfile=to+"-"+getpid()+"-"+time();
  if(!Stdio.cp(from,tmpfile))
    fail("copy(%s,%s)",from,tmpfile);

  /* Chown and chgrp not implemented yet */
  chmod(tmpfile,mode);

  /* Need to rename the old file to .old */
  if(file_stat(to))
  {
    rm(to+".old"); // Ignore errors
#if constant(hardlink)
    if( catch { hardlink(to,to+".old"); })
#endif 
      mv(to,to+".old");
  }
  if(!mv(tmpfile,to))
    fail("mv(%s,%s)",tmpfile,to);

  return 1;
}

int install_file(string from,
		 string to,
		 void|int mode,
		 void|int dump)
{
  int ret;
  if(query_num_arg() == 2)
    ret=low_install_file(from,to);
  else
    ret=low_install_file(from,to,mode);

  array(string) tmp=to/".";
  string ext=tmp[-1];

  if((ret || redump_all) && dump)
  {
    switch(ext)
    {
      case "pike":
	if(glob("*/master.pike",to)) break;

      case "pmod":
	to_dump+=({to});
    }
  }

  // This magic deletes the remnants of static modules
  // when dynamic modules are installed.
  if(ret && ext == "so")
  {
    tmp[-1]="pmod";
    files_to_delete+=({ tmp*"." });
  }else{
    files_to_not_delete+=({ to });
  }
  return ret;
}

string stripslash(string s)
{
  while(strlen(s)>1 && s[-1]=='/') s=s[..strlen(s)-2];
  return s;
}


void install_dir(string from, string to,int dump)
{
  from=stripslash(from);
  to=stripslash(to);

  installed_files++;
  mkdirhier(to);
//  werror("\nFOO (from=%s, cwd=%s)\n",from,getcwd());
  foreach(get_dir(from),string file)
  {
    if(file=="CVS") continue;
    if(file[..1]==".#") continue;
    if(file[0]=='#' && file[-1]=='#') continue;
    if(file[-1]=='~') continue;
    mixed stat=file_stat(combine_path(from,file));
    if (stat) {
      if(stat[1]==-2) {
	install_dir(combine_path(from,file),combine_path(to,file),dump);
      } else if (stat[0] & 0111) {
	// Executable
	install_file(combine_path(from,file),combine_path(to,file),0755,dump);
      } else {
	// Not executable
	install_file(combine_path(from,file),combine_path(to,file),0644,dump);
      }
    } else {
      werror(sprintf("\nstat:0, from:%O, file:%O, combined:%O\n",
		     from, file, combine_path(from, file)));
    }
  }
}

void install_header_files(string from, string to)
{
  installed_files++;
  from=stripslash(from);
  to=stripslash(to);
  mkdirhier(to);
  foreach(get_dir(from),string file)
    {
      if(file[..1]==".#") continue;
      if(file[-1]!='h' || file[-2]!='.') continue;
      install_file(combine_path(from,file),combine_path(to,file));
    }
}

mapping vars=([]);

object reg;
string regexp;

string regquote(string s)
{
  while(s[-1] == '/' || s[-1]=='\\') s=s[..strlen(s)-2];
  return
    replace(s,
	    ({".","[","]","*","\\","(",")","|","+"}),
	    ({"\\.","\\[","\\]","\\*","\\\\","\\(","\\)","\\|","\\+"}) );
}

string globify(string s)
{
  if(s[-1]=='/') s=s[..strlen(s)-2];
  return s+"*";
}

string fakeroot(string s)
{
  if(!vars->fakeroot) return s;
  if(!reg)
  {
    reg=Regexp(regexp=sprintf("^([^/])%{|(%s)%}",
			      Array.map(
				({
				  getcwd(),
				    vars->LIBDIR_SRC,
				    vars->SRCDIR,
				    vars->TMP_BINDIR,
				    vars->MANDIR_SRC,
				    vars->TMP_LIBDIR,
				    vars->fakeroot,
				    }),regquote)));
  }
  if(reg->match(s)) return s;
  return vars->fakeroot+s;
}

string export_base_name;

int mklink(string from, string to)
{
#if constant(symlink)
  catch  {
    symlink(from, to);
    return 1;
  };
#endif
  return 0;
}

string helptext=#"Usage: $TARFILE [options] [variables]

Options:
  -h, --help            Display this help and exit.
  -v, --version         Display version information and exit.
  --features            Display features and exit.
  --interactive         Interactive installation (default).
  --new-style           Install in <prefix>/pike/<ver>/{lib,include,bin}.
  --traditional         Install in <prefix>/{lib/pike,include/pike,bin}.

Variables:
  prefix=<path>         Install pike files here (/usr/local).
  pike_name=<path>      Create a symlink to pike here (<prefix>/bin/pike).
";

string translate(string filename, mapping translator)
{
  return translator[filename] ||
    combine_path(translate(dirname(filename),translator),basename(filename));
};

void tarfilter(string filename)
{
  ((program)combine_path(__FILE__, "..", "tarfilter"))()->
    main(3, ({ "tarfilter", filename, filename }));
}

void do_export()
{
#ifdef __NT__
  status("Creating",export_base_name+".burk");
  Stdio.File p=Stdio.File(export_base_name+".burk","wc");
  string msg="   Loading installation script, please wait...";
  p->write("w%4c%s",strlen(msg),msg);

#define TRANSLATE(X,Y) combine_path(".",X) : Y
  string tmpdir="~piketmp";

  mapping translator = ([
    TRANSLATE(vars->LIBDIR_SRC,tmpdir+"/lib"),
    TRANSLATE(vars->SRCDIR,tmpdir+"/src"),
    TRANSLATE(vars->TMP_BINDIR,tmpdir+"/bin"),
    TRANSLATE(vars->MANDIR_SRC,tmpdir+"/man"),
    TRANSLATE(vars->TMP_LIBDIR,tmpdir+"/build/lib"),
    "":tmpdir+"/build",
    ]);
    
  
  array(string) translated_names = Array.map(to_export, translate, translator);
  array(string) dirs=Array.uniq(Array.map(translated_names, dirname));
  while(1)
  {
    array(string) d2=Array.map(dirs, dirname) - dirs;
    if(!sizeof(d2)) break;
    dirs+=Array.uniq(d2);
  }
  dirs-=({""});
  sort(dirs);

  foreach(dirs, string dir) p->write("d%4c%s",strlen(dir),dir);
  foreach(Array.transpose(  ({ to_export, translated_names }) ), [ string file, string file_name ])
    {
      status("Adding",file);
      if (string f=Stdio.read_file(file)) {
	p->write("f%4c%s%4c",strlen(file_name),file_name,strlen(f));
	p->write(f);
      } else {
	//  Huh? File could not be found.
	werror("-------------------\n"
	       "Warning: Could not add file: %s. File not found!\n"
	       "-------------------\n", file);
      }
    }

  /* FIXME, support $INSTALL_SCRIPT (or similar) */

#define TRVAR(X) translate(combine_path(vars->X,"."), translator)

  array(string) env=({
//    "PIKE_MODULE_PATH="+TRVAR(TMP_LIBDIR)+"/modules:"+TRVAR(LIBDIR_SRC)+"/modules",
//    "PIKE_PROGRAM_PATH=",
//    "PIKE_INCLUDE_PATH="+TRVAR(LIBDIR_SRC)+"/include",
    });

    
  foreach(env, string e)
    p->write("e%4c%s",strlen(e),e);

#define RELAY(X) " " #X "=" + TRVAR(X)+

  string cmd=
    replace(translate("pike.exe", translator),"/","\\")+
    " -m"+translate("master.pike", translator)+
    " -DNOT_INSTALLED" +
    " "+translate( combine_path(vars->TMP_BINDIR,"install.pike"), translator)+
    RELAY(TMP_LIBDIR)
    RELAY(LIBDIR_SRC)
    RELAY(SRCDIR)
    RELAY(TMP_BINDIR)
    RELAY(MANDIR_SRC)
    " TMP_BUILDDIR="+translate("", translator)+
    (((vars->PIKE_MODULE_RELOC||"") != "")? " PIKE_MODULE_RELOC=1":"")+
    " $" // $ = @argv
    ;
  
  p->write("s%4c%s",strlen(cmd),cmd);

  array(string) to_delete=translated_names + ({translate("pike.tmp",translator)});
  to_delete=Array.uniq(to_delete);
  to_delete+=reverse(dirs);
  /* Generate cleanup */
  foreach(to_delete, string del)
    p->write("D%4c%s",strlen(del),del);

  p->write("q\0\0\0\0");
  p->close("rw");

  if(last_len)
  {
    status(0,"");
    status(0,"");
  }

#else
  export=0;

  cd("..");

  string tmpname=sprintf("PtmP%07x",random(0xfffffff));

  status("Creating","script glue");

  Stdio.write_file(tmpname+".x",
		   "#!/bin/sh\n"
#"TARFILE=\"$1\"; shift
ARGS=''

INSTALL_SCRIPT='bin/install.pike'

while [ $# != 0 ]
do
    case \"$1\" in
              -v|\\
       --version) echo \""+version()+
#" Copyright (C) 1994-2003 Fredrik Hübinette and
Roxen Internet Software AB Pike comes with ABSOLUTELY NO WARRANTY;
This is free software and you are welcome to redistribute it under
certain conditions; Read the files COPYING and DISCLAIMER in the
Pike distribution for more details.
\";
		  rm -f "+tmpname+#".x
                  exit 0 ;;

              -h|\\
          --help) echo \"" + helptext + #"\"
		  rm -f "+tmpname+#".x
                  exit 0 ;;

      --features) echo \"" + Tools.Install.features()*"\n" + #"\"
		  rm -f "+tmpname+#".x
                  exit 0 ;;

              -s|\\
        --script) shift
                  INSTALL_SCRIPT=\"$1\" ;;

               *) ARGS=\"$ARGS '`echo \\\"$1\\\" | sed -e \\\"s/'/'\\\\\\\"'\\\\\\\"'/g\\\"`'\" ;;
    esac
    shift
done
"
		   "echo \"   Loading installation script, please wait...\"\n"
		   "tar xf \"$TARFILE\" "+tmpname+".tar.gz\n"
		   "gzip -dc "+tmpname+".tar.gz | tar xf -\n"
		   "rm -rf "+tmpname+".tar.gz\n"
		   "( cd '"+export_base_name+".dir'\n"
		   "  eval \"build/pike -mmaster.pike -DNOT_INSTALLED "
		                "\\\"$INSTALL_SCRIPT\\\" \\\n"
		   "  TMP_LIBDIR=\\\"build/lib\\\"\\\n"
		   "  LIBDIR_SRC=\\\"lib\\\"\\\n"
		   "  SRCDIR=\\\"src\\\"\\\n"
		   "  TMP_BINDIR=\\\"bin\\\"\\\n"
		   "  TMP_BUILDDIR=\\\"build\\\"\\\n"
		   "  MANDIR_SRC=\\\"man\\\"\\\n"
		   "  PIKE_MODULE_RELOC=\\\"" + vars->PIKE_MODULE_RELOC +
		                       "\\\"\\\n"
		   "  $ARGS\"\n"
		   ")\n"
		   "rm -rf '"+export_base_name+".dir' "+tmpname+".x\n"
    );
  chmod(tmpname+".x",0755);
  string script=sprintf("#!/bin/sh\n"
			"tar xf \"$0\" %s.x\n"
			"exec ./%s.x \"$0\" \"$@\"\n",tmpname,tmpname,tmpname);
  if(strlen(script) >= 100)
  {
    werror("Script too long!!\n");
    exit(1);
  }

  array(string) parts=(script/"/");
  mkdirhier( parts[..sizeof(parts)-2]*"/");
  Stdio.write_file(script,"");

  to_export=Array.map(to_export,
	      lambda(string s)
	      {
		return combine_path(export_base_name+".dir",s);
	      });


  string tmpmsg=".";

  string tararg="cf";
  foreach(to_export/50.0, array files_to_tar)
    {
      status("Creating",tmpname+".tar",tmpmsg);
      tmpmsg+=".";
      Process.create_process(({"tar",tararg,tmpname+".tar"})+ files_to_tar)
	->wait();
      tararg="rf";
    }

  status("Filtering to root/root ownership", tmpname+".tar");
  tarfilter(tmpname+".tar");

  status("Creating",tmpname+".tar.gz");

  Process.create_process(({"gzip","-9",tmpname+".tar"}))->wait();

  to_export=({script,tmpname+".x",tmpname+".tar.gz"});

  status("Creating",export_base_name);

  Process.create_process( ({ "tar","cf", export_base_name})+ to_export)
    ->wait();

  status("Filtering to root/root ownership", export_base_name);
  tarfilter(export_base_name);

  chmod(export_base_name,0755);

  status("Cleaning up","");

  Process.create_process( ({ "rm","-rf",
			       export_base_name+".dir",
			       export_base_name+".x"
			       }) ) ->wait();


#endif
  status1("Export done");

  exit(0);
}

#ifdef USE_GTK
object window1;
object vbox1;
object label4;
object frame1;
object hbox1;
object vbox2;
object label3;
object table1;
object entry1;
object entry2;
object label1;
object label2;
object button4;
object button5;
object hbuttonbox1;
object button1;
object button2;
object label5;
object label6;
object label7;
object vbox3;

#define PS pack_start
#define AT attach

void do_abort()
{
  // FIXME
  werror("Installation aborted.\n");
  exit(1);
}

void update_entry2()
{
  entry2->set_text( combine_path( entry1 -> get_text(), "bin/pike") );
}

void set_filename(array ob, object button)
{
  object selector=ob[0];
  object entry=ob[1];
  entry->set_text(selector->get_filename());
  if(entry == entry1)
    update_entry2();
  destruct(selector);
}

void selectfile(object entry, object button)
{
  object selector;
  selector=GTK.FileSelection("Pike installation prefix");
  selector->set_filename(entry->get_text());
  selector->ok_button()->signal_connect("clicked",set_filename, ({selector, entry}) );
  selector->cancel_button()->signal_connect("clicked",destruct,selector);
  selector->show();
}

void do_exit()
{
  exit(0);
}

void cancel()
{
  werror("See you another time!\n");
  exit(0);
}

void proceed()
{
  pre_install(({}));
  label6->set_text("Click Ok to exit installation program.");
  hbuttonbox1->add(button1=GTK.Button("Ok")->show());
  button1->signal_connect("pressed",do_exit,0);
}

int next()
{
  vars->prefix = entry1->get_text();
  vars->pike_name = entry2->get_text();
  install_type="--new_style";

  destruct(table1);
  
  vbox2->PS(vbox3=GTK.Vbox(0,0)->show(),1,1,0);

  vbox3->PS(label7=GTK.Label("---head---")->show(),0,0,0);
  vbox3->PS(label1=GTK.Label("---action---")->show(),0,0,0);
  vbox3->PS(label2=GTK.Label("----dir-----")->show(),0,0,0);
  vbox3->PS(label5=GTK.Label("----file----")->show(),0,0,0);
  vbox3->PS(label6=GTK.Label("----msg----")->show(),0,0,0);
  destruct(button1);
  destruct(button2);

  call_out(proceed, 0);
  return 1;
}

void begin_wizard(array(string) argv)
{
  // FIXME:
  // We should display the GPL licence and make the user
  // click 'agree' first
  //
  GTK.setup_gtk(argv);
  window1=GTK.Window(GTK.WINDOW_TOPLEVEL)
    ->set_title(version()+" installer")
    ->add(vbox1=GTK.Vbox(0,0)
	  ->PS(label4=GTK.Label(version()+" installer")
	       ->set_justify(GTK.JUSTIFY_CENTER),0,0,10)
	  ->PS(frame1=GTK.Frame()
	       ->set_shadow_type(GTK.SHADOW_IN)
	       ->set_border_width(11)
	       ->add(hbox1=GTK.Hbox(0,0)
		     ->PS(GTK.Pixmap(GTK.Util.load_image(combine_path(vars->SRCDIR,"install-welcome"))->img),0,0,0)
		     ->PS(vbox2=GTK.Vbox(0,0),1,1,0)
		       ),1,1,0)
	  ->PS(hbuttonbox1=GTK.HbuttonBox()
	       ->set_border_width(15)
	       ->add(button1=GTK.Button("Cancel"))
	       ->add(button2=GTK.Button("Install Pike >>"))
	       ,0,1,0));

	  
  vbox2->PS(label3=GTK.Label("Welcome to the interactive "+version()+" installer.")
	    ->set_justify(GTK.JUSTIFY_CENTER),1,1,0)
    ->PS(table1=GTK.Table(3,2,0)
	 ->set_border_width(19)
	 ->AT(entry1=GTK.Entry(),
	      1,2,0,1,GTK.Fill | GTK.Expand,0,0,0)
	 ->AT(entry2=GTK.Entry(),
	      1,2,1,2,GTK.Fill | GTK.Expand,0,0,0)
	 ->AT(label1=GTK.Label("Install prefix: ")
	      ->set_justify(GTK.JUSTIFY_RIGHT),
	      0,1,0,1,GTK.Expand,0,0,0)
	 ->AT(label2=GTK.Label("Pike binary name: ")
	      ->set_justify(GTK.JUSTIFY_RIGHT),
	      0,1,1,2,GTK.Expand,0,0,0)
	 ->AT(button4=GTK.Button("Browse"),
	      2,3,1,2,0,0,0,0)
	 ->AT(button5=GTK.Button("Browse"),
	      2,3,0,1,0,0,0,0),0,0,0);

  vbox2->show_all();

  entry1->set_text(prefix);
  entry2->set_text(vars->pike_name ||
		   combine_path(vars->exec_prefix||combine_path(prefix, "bin"),"pike"));

  entry1->signal_connect("focus_out_event",update_entry2,0);
  button1->signal_connect("pressed",cancel,0);
  button2->signal_connect("pressed",next,0);
  button4->signal_connect("pressed",selectfile,entry2);
  button5->signal_connect("pressed",selectfile,entry1);

  window1->show_all();
}
#endif


int traditional;
string prefix;
string exec_prefix;
string lib_prefix;
string include_prefix;
string man_prefix;
string lnk;
string old_exec_prefix;
object interactive;
string install_type="--interactive";


int pre_install(array(string) argv)
{
  
  prefix=vars->prefix || "/usr/local";
  
  if(!vars->TMP_BINDIR)
    vars->TMP_BINDIR=combine_path(vars->SRCDIR,"../bin");

  if(!vars->TMP_BUILDDIR) vars->TMP_BUILDDIR=".";

  while(1)
  {
  switch(install_type)
  {
    case "--traditional":
      exec_prefix=vars->exec_prefix||(prefix+"/bin/");
      lib_prefix=vars->lib_prefix||(prefix+"/lib/pike/");
      include_prefix=combine_path(prefix,"include","pike");
      man_prefix=vars->man_prefix||(prefix+"/man/");
      break;

    case "--interactive":

#ifdef USE_GTK
      catch  {
	if(!no_gui)
	{
#ifndef __NT__ /* We are using GTK on Win32!! no DISPLAY required */
	  if(getenv("DISPLAY"))
#endif
	  {
	    begin_wizard(argv);
	    return -1; 
	  }
	}
      };
#endif

      status1("");
  
      interactive=Tools.Install.Readline();
      interactive->set_cwd("../");

      write("   Welcome to the interactive "+version()+
	    " installation script.\n"
	    "\n" +
	    (interactive->get_input_controller()->dumb ?
	     "   The script will guide you through the installation process by asking\n"
	     "   a few questions. You will be able to confirm your settings before\n"
	     "   the installation begins.\n"
	     :
	     "   The script will guide you through the installation process by asking\n"
	     "   a few questions. Whenever you input a path or a filename, you may use\n"
	     "   the <tab> key to perform filename completion. You will be able to\n"
	     "   confirm your settings before the installation begins.\n")
	    );
      
      string confirm, bin_path = vars->pike_name;
      do {
	write("\n");
	
//	werror("PREFIX: %O\n",prefix);
//	if(!vars->prefix)
	prefix=interactive->edit_directory(prefix,"Install prefix: ");
	prefix = interactive->absolute_path(prefix);

	if(!vars->pike_name)
	{
#if constant(symlink)	
	  bin_path=interactive->edit_filename
		   (combine_path(vars->exec_prefix ||
				 combine_path(prefix, "bin"),
				 "pike"), "Pike binary name: ");
#else
	  bin_path=combine_path("/",getcwd(),prefix,"pike",
				replace(version()-"Pike v"," release ","."),
				"bin","pike");
#endif
	}

	bin_path = interactive->absolute_path(bin_path);
	
	write("\n");
	confirm =
	  lower_case(interactive->
		     edit("", "Are the settings above correct [Y/n/quit]? "));
	if(confirm == "quit")
	{
	  // Maybe clean up?
	  destruct(interactive);
	  exit(0);
	}

      } while(!(confirm == "" || confirm == "y"));

      write("\n");
      
      vars->pike_name = bin_path;
      
      destruct(interactive);
      install_type="--new-style";
//      trace(2);
      continue;

    case "--export":
      string ver=replace(replace(version()," ","-"),"-release-",".");
#if constant(uname)
      mixed u=uname();
      if(u->sysname=="AIX")
      {
	export_base_name=sprintf("%s-%s-%s.%s",
				 ver,
				 uname()->sysname,
				 uname()->version,
				 uname()->release);
      }else{
	export_base_name=sprintf("%s-%s-%s-%s",
				 ver,
				 uname()->sysname,
				 uname()->release,
				 uname()->machine);
      }
      export_base_name=replace(export_base_name,
			       ({ "/", " " }), ({ "-", "-" }) );
#else
      export_base_name=ver;
#endif

      status1("Building export %s\n",export_base_name);

#ifndef __NT__
      mkdirhier(export_base_name+".dir");

      mklink(vars->LIBDIR_SRC,export_base_name+".dir/lib");
      mklink(vars->SRCDIR,export_base_name+".dir/src");
      mklink(getcwd(),export_base_name+".dir/build");
      mklink(vars->TMP_BINDIR,export_base_name+".dir/bin");
      mklink(vars->MANDIR_SRC,export_base_name+".dir/man");

      cd(export_base_name+".dir");

      vars->TMP_LIBDIR="build/lib";
      vars->LIBDIR_SRC="lib";
      vars->SRCDIR="src";
      vars->TMP_BINDIR="bin";
      vars->MANDIR_SRC="man";
      vars->TMP_BUILDDIR="build";

#endif

      export=1;
      to_export+=({ combine_path(vars->TMP_BINDIR,"install.pike") });
      
    case "":
    default:
    case "--new-style":
      if(!(lnk=vars->pike_name) || !strlen(lnk)) {
	lnk=combine_path(vars->exec_prefix || combine_path(vars->prefix, "bin"),"pike");
	old_exec_prefix=vars->exec_prefix; // to make the directory for pike link
      }
      prefix=combine_path("/",getcwd(),prefix,"pike",
			  replace(version()-"Pike v"," release ","."));
      exec_prefix=combine_path(prefix,"bin");
      lib_prefix=combine_path(prefix,"lib");
      include_prefix=combine_path(prefix,"include","pike");
      man_prefix=combine_path(prefix,"man");
      break;
  }
  break;
  }

  do_install();
  return 0;
}

// Create a master.pike with the correct lib_prefix
void make_master(string dest, string master, string lib_prefix,
		 string include_prefix, string|void share_prefix)
{
  status("Finalizing",master);
  string master_data=Stdio.read_file(master);
  master_data=replace(master_data,
		      ({"¤lib_prefix¤", "¤include_prefix¤", "¤share_prefix¤"}),
		      ({replace(lib_prefix,"\\","\\\\"),
			replace(include_prefix,"\\","\\\\"),
			replace(share_prefix||"¤share_prefix¤", "\\", "\\\\"),
		      }));
  if((vars->PIKE_MODULE_RELOC||"") != "")
    master_data=replace(master_data,"#undef PIKE_MODULE_RELOC","#define PIKE_MODULE_RELOC 1");
  Stdio.write_file(dest, master_data);
  status("Finalizing",master,"done");
}

/* dump modules (and master) */
void dump_modules()
{
  string master=combine_path(lib_prefix,"master.pike");
  mixed s1=file_stat(master);
  mixed s2=file_stat(master+".o");
  mapping(string:mapping(string:string)) options = ([ 
    "env":getenv()-([
      "PIKE_PROGRAM_PATH":"",
      "PIKE_MODULE_PATH":"",
      "PIKE_INCLUDE_PATH":"",
      "PIKE_MASTER":"",
      ]) ]); 
  
  
  if(!s1 || !s2 || s1[3]>=s2[3] || redump_all)
  {
    Process.create_process( ({fakeroot(pike),"-m",
				  combine_path(vars->SRCDIR,"dumpmaster.pike"),
				@(vars->fakeroot?({"--fakeroot="+
                                                     vars->fakeroot}):({})),
				master}), options)->wait();
  }

  if(sizeof(to_dump))
  {
    rm("dumpmodule.log");
    
    foreach(to_dump, string mod)
      rm(mod+".o");
      
    /* Dump 25 modules at a time */

    array cmd=({fakeroot(pike) });

    if(vars->fakeroot)
      cmd+=({
	sprintf("-DPIKE_FAKEROOT=%O",vars->fakeroot),
	  sprintf("-DPIKE_FAKEROOT_OMIT=%O",
		  Array.map( ({
		    getcwd(),
		      vars->LIBDIR_SRC,
		      vars->SRCDIR,
		      vars->TMP_BINDIR,
		      vars->MANDIR_SRC,
		      vars->TMP_LIBDIR,
		      vars->fakeroot,
		      }), globify)*":"),
	  "-m",combine_path(vars->TMP_LIBDIR,"master.pike")
	  });

    cmd+=({ combine_path(vars->SRCDIR,"dumpmodule.pike"),

#ifdef USE_GTK
	      label1?"--distquiet":
#endif
      "--quiet"});

//      werror("%O\n",cmd);

    int offset = 1;
    foreach(to_dump/25.0, array delta_dump)
    {
      catch {
	Process.create_process(cmd +
			       ( istty() ? 
				 ({
				   "--progress-bar",
				   sprintf("%d,%d", offset, sizeof(to_dump))
				 }) : ({"--quiet"}) ) +
			       delta_dump, options)->wait();
      };

      offset += sizeof(delta_dump);
    }
      
    if(progress_bar)
      /* The last files copied does not really count (should
	 really be a third phase)... */
      progress_bar->set_phase(1.0, 0.0);
    
    status_clear(1);
  }
}

void do_install()
{
  pike=combine_path(exec_prefix,"pike");
  if(!export)
  {
    status1("Installing Pike in %s, please wait...\n", fakeroot(prefix));
  }
  catch {
    files_to_install = (int)Stdio.read_file(combine_path(vars->TMP_BUILDDIR,"num_files_to_install"));
    
    if(!export && files_to_install)
      progress_bar =
	Tools.Install.ProgressBar("Installing", 0, files_to_install, 0.0, 0.2);
  };

  mixed err=catch {

    /* Ugly way to detect NT installation */
    string pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike");
    if(Stdio.file_size(pike_bin_file) < 10000 &&
       file_stat(pike_bin_file+".exe"))
    {
      pike_bin_file+=".exe";
      pike+=".exe";
    }

    if(export)
    {
      to_export+=({pike_bin_file});
    }else{
      status("Finalizing",pike_bin_file);
      string pike_bin=Stdio.read_file(pike_bin_file);
      int pos=search(pike_bin,MASTER_COOKIE);
      
      if(pos>=0)
      {
	status("Finalizing",pike_bin_file,"...");
	Stdio.write_file(pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike.tmp"),pike_bin);
	Stdio.File f=Stdio.File(pike_bin_file,"rw");
	f->seek(pos+strlen(MASTER_COOKIE));
	f->write(combine_path(lib_prefix,"master.pike"));
	f->close();
	status("Finalizing",pike_bin_file,"done");
      }else{
	werror("Warning! Failed to finalize master location!\n");
      }
      if(install_file(pike_bin_file,pike)) redump_all=1;
    }

#ifdef __NT__
    // Copy needed DLL files (like libmySQL.dll if available).
    foreach(glob("*.dll", get_dir(vars->TMP_BUILDDIR)), string dll_name)
      install_file(combine_path(vars->TMP_BUILDDIR, dll_name),
		   combine_path(exec_prefix, dll_name));
    // Copy the Program Database (debuginfo)
    if(file_stat(combine_path(vars->TMP_BUILDDIR, "pike.pdb")))
      install_file(combine_path(vars->TMP_BUILDDIR, "pike.pdb"),
		   combine_path(exec_prefix, "pike.pdb"));
#endif
    install_file(combine_path(vars->TMP_BUILDDIR,"hilfe"),combine_path(exec_prefix,"hilfe"));
    string master=combine_path(vars->LIBDIR_SRC,"master.pike.in");
    
    if(export)
    {
      make_master("master.pike", master, "build/lib", "build", "lib");
      to_export+=({master,
		   "master.pike",
		   //combine_path(vars->TMP_BUILDDIR,"master.pike"),
		   combine_path(vars->SRCDIR,"COPYING"),
		   combine_path(vars->SRCDIR,"install-welcome"),
		   combine_path(vars->SRCDIR,"dumpmaster.pike"),
		   combine_path(vars->SRCDIR,"dumpmodule.pike")
      });
    }else{
      make_master(combine_path(vars->TMP_LIBDIR,"master.pike"), master, 
		  lib_prefix, include_prefix);
    }
    
    install_dir(vars->TMP_LIBDIR,lib_prefix,1);
    install_dir(vars->LIBDIR_SRC,lib_prefix,1);
    
    install_header_files(vars->SRCDIR,include_prefix);
    install_header_files(vars->TMP_BUILDDIR,include_prefix);
    
    install_file(combine_path(vars->TMP_BUILDDIR,"modules/dynamic_module_makefile"),
		 combine_path(include_prefix,"dynamic_module_makefile"));
    install_file(combine_path(vars->TMP_BUILDDIR,"aclocal"),
		 combine_path(include_prefix,"aclocal.m4"));
    
    if(file_stat(vars->MANDIR_SRC))
    {
//      trace(9);
//      _debug(5);
      install_dir(vars->MANDIR_SRC,combine_path(man_prefix,"man1"),0);
    }
  };


  status_clear();

  if(err) throw(err);

  catch {
    Stdio.write_file(combine_path(vars->TMP_BUILDDIR,"num_files_to_install"),
		     sprintf("%d\n",installed_files));
    to_export+=({ combine_path(vars->TMP_BUILDDIR,"num_files_to_install") });
  };

  files_to_install=0;

  if(export)
  {
    do_export();
  }else{
    dump_modules();
    
    // Delete any .pmod files that would shadow the .so
    // files that we just installed. For a new installation
    // this never does anything. -Hubbe
    Array.map(files_to_delete - files_to_not_delete,rm);
    
#if constant(symlink)
    if(lnk)
    {
      status("Creating",lnk);
      mixed s=file_stat(fakeroot(lnk),1);
      if(s)
      {
	if(!mv(fakeroot(lnk),fakeroot(lnk+".old")))
	{
	  werror("Failed to move %s\n",lnk);
	  exit(1);
	}
      }
      if (old_exec_prefix) {
	mkdirhier(fakeroot(old_exec_prefix));
      }
      mkdirhier(fakeroot(dirname(lnk)));
      symlink(pike,fakeroot(lnk));
      status("Creating",lnk,"done");
    }
#endif
  }

  progress_bar = 0;
  status1("Pike installation completed successfully.");
}

int main(int argc, array(string) argv)
{
  foreach(Getopt.find_all_options(argv,aggregate(
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"notty",Getopt.NO_ARG,({"-t","--notty"})}),
    ({"--interactive",Getopt.NO_ARG,({"-i","--interactive"})}),
    ({"--new-style",Getopt.NO_ARG,({"--new-style"})}),
    ({"no-gui",Getopt.NO_ARG,({"--no-gui","--no-x"})}),
    ({"--export",Getopt.NO_ARG,({"--export"})}),
    ({"--traditional",Getopt.NO_ARG,({"--traditional"})}),
    )),array opt)
    {
      switch(opt[0])
      {
	case "no-gui":
	  no_gui=1;
	  break;

	case "help":
	  write(helptext);
	  exit(0);

	case "notty":
	  istty_cache=-1;
	  break;

	default:
	  install_type=opt[0];
      }
    }

  argv=Getopt.get_args(argv);

  foreach(argv[1..], string foo)
    if(sscanf(foo,"%s=%s",string var, string value)==2)
      vars[var]=value;

  /* Some magic for the fakeroot stuff */
  string tmp=vars->fakeroot;
  m_delete(vars,"fakeroot");
  if(tmp && tmp!="")
  {
    if(tmp[-1]=='/' || tmp[-1]=='\\')
      tmp=tmp[..sizeof(tmp)-2];

    /* Create the fakeroot if it doesn't exist
    /* This must be done with fakeroot unset since
     * it would create fakeroot/fakeroot otherwise
     */
    mkdirhier(tmp);
    vars->fakeroot=tmp;
  }
  return pre_install(argv);
}
