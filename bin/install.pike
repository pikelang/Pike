#! /usr/bin/env pike
#charset iso-8859-1

// Pike installer and exporter.

// Windows installer FIXMEs:
//
// o  Want version in the title that gets entered into the Windows
//    installed programs list (but not e.g. for the install dir).
// o  Add cleanup rule for the generated master.pike for uninstall.
// o  Remove meaningless "please click next" dialog.
// o  Include dumped files.
// o  Start menu entries.
// o  Make sure old installer regstry keys are removed.
// o  Separate shell icon for .pmod
// o  Refresh shell when icons are installed. Now a explorer restart
//    is needed to see them.
// o  Upgrading an existing installation seems flaky - the old one has
//    been observed to still be considered installed according to the
//    installed program list (in the same location and hence
//    overwritten).
//
// Note: It's not possible to change the .msi icon.

#define USE_GTK

#ifdef USE_GTK
#if constant(GTK2.parse_rc)
#define USE_GTK2
#define GTK GTK2
#else
#if !constant(GTK.parse_rc)
#undef USE_GTK
#endif
#endif
#endif

// #define GENERATE_WIX_UI
// #define GENERATE_WIX_ACTIONS
// #define WIX_DEBUG

constant pike_upgrade_guid = "6e40542b-dcfc-49fc-ad8a-b0f978e2935e";

constant line_feed = Standards.XML.Wix.line_feed;
constant WixNode = Standards.XML.Wix.WixNode;
constant Directory = Standards.XML.Wix.Directory;

string version_str = sprintf("%d.%d.%d",
			     __REAL_MAJOR__,
			     __REAL_MINOR__,
			     __REAL_BUILD__);
#if constant(Standards.UUID.make_version3)
#define SUPPORT_WIX
string version_guid = Standards.UUID.make_version3(version_str,
						   pike_upgrade_guid)->str();
Directory root = Directory("SourceDir",
			   Standards.UUID.UUID(version_guid)->encode(),
			   "TARGETDIR");
#else /* !constant(Standards.UUID.make_version3) */
#warning Standards.UUID.make_version3 not available.
#warning Wix support disabled.
#endif

int last_len;
int redump_all;
string pike;

int no_gui;
int verbose;
int no_autodoc;
string include_crt;

Tools.Install.ProgressBar progress_bar;

// for progress bar
int files_to_install;
int installed_files;

// the nt scripts depends on this value
// (incidentally defined elsewhere in the C code too)
#define MASTER_COOKIE __master_cookie

int istty_cache;
int istty()
{
  if(!istty_cache)
  {
#ifdef __NT__
    istty_cache=1;
#else
    istty_cache=!!Stdio.stdin->tcgetattr();
#endif
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
}

void status1_impl(string fmt, mixed ... args)
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

  write(fmt+"\n", @args);
}

function(string, mixed ... : void) status1 = status1_impl;

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

void error_msg (string msg, mixed... args)
{
  if (last_len) {
    write ("\n");
    last_len = 0;
  }
  werror (msg, @args);
}

void status_impl(string|void doing, void|string file, string|void msg)
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
  if(sizeof(file)>45)
    file=".."+file[sizeof(file)-44..];

  if(msg) file+=" "+msg;
  if(doing) file=doing+": "+file;
  string s="\r   "+file;
  int t=sizeof(s);
  if(t<last_len) s+=" "*(last_len-t);
  last_len=t;
  write(s);
}

function (string|void, void|string, string|void : void) status = status_impl;

void status_clear(void|int all)
{
    if(all)
	last_len = 75;
    status(0,"");
    status(0,"");
}

#ifdef GENERATE_WIX_UI

class Dialog
{
  string id;
  string banner;

  class Control
  {
    string id;
    int x;
    int y;
    int width;
    int height;
    int disabled;
    constant type = "";
    string font = "\\VSI_MS_Sans_Serif13.0_0_0";
    string text;

    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ([
	"Type":type,
	"Id":id,
	"X":(string)x,
	"Y":(string)y,
	"Width":(string)width,
	"Height":(string)height,
      ]);
      if (disabled) {
	res->Disabled="yes";
      }
      return res;
    }

    array(WixNode) get_children()
    {
      if (text) {
	return ({
	  WixNode("Text", ([]), sprintf("{%s}%s", font, text)),
	});
      }
      return ({});
    }

    WixNode gen_xml()
    {
      mapping(string:string) attrs = get_attrs();
      foreach(attrs; string attr; string val) {
	if (!stringp(val)) {
	  error("Bad attributes for control: %O\n", attrs);
	}
      }
      WixNode res = WixNode("Control", attrs);
      foreach(get_children(), WixNode subnode) {
	res->add_child(subnode);
      }
      return res;
    }
  }

  class TextLabel
  {
    inherit Control;
    constant type = "Text";
    int x = 18;
    int height = 12;
    int transparent = 0;
    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      if (transparent) res->Transparent = "yes";
      return res;
    }
  }

  class BodyText
  {
    inherit TextLabel;
    int y = 63;
    int width = 342;
    int height = 24;
    string id = "BodyText";
  }

  class BannerText
  {
    inherit TextLabel;
    string id = "Banner";
    int x = 9;
    int y = 9;
    int width = 306;
    int height = 33;
    int transparent = 1;
    string font = "\\VSI_MS_Sans_Serif16.0_1_0";
    protected void create()
    {
      text = banner;
    }
  }

  class BannerBmp
  {
    inherit Control;
    constant type = "Bitmap";
    string id = "BannerBmp";
    int x = 0;
    int y = 0;
    int width = 375;
    int height = 52;
    string bitmap = "Pike_banner";
    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      res->TabSkip = "no";
      res->Text = bitmap;
      return res;
    }
  }

  class Line
  {
    inherit Control;
    constant type = "Line";
    string id = "Line";
    int x = 0;
    int y = 52;
    int width = 375;
    int height = 6;
    string text = "MsiHorizontalLine";
  }

  class Line2
  {
    inherit Line;
    string id = "Line2";
    int y = 252;
  }

  class Button
  {
    inherit Control;
    constant type = "PushButton";
    int y = 261;
    int width = 66;
    int height = 18;
  }

  class CancelButton
  {
    inherit Button;
    int x = 156;
    string text = "Cancel";
    string id = "Cancel";

    mapping(string:string) get_attrs()
    {
      mapping res = ::get_attrs();
      res->Cancel = "yes";
      return res;
    }
  }

  class PrevButton
  {
    inherit Button;
    int x = 228;
    string text = "< &Back";
    string id = "Prev";

    array(WixNode) get_children()
    {
      string prev_dialog_prop = sprintf("%s_PrevArgs", Dialog::id);
      if (disabled) {
	return ::get_children();
      }
      return ({
	@::get_children(),
	WixNode("Publish", ([
		  "Event":"NewDialog",
		  "Value":sprintf("[%s]", prev_dialog_prop),
		]),
		sprintf("%s<>\"\"", prev_dialog_prop)),
	WixNode("Condition", ([ "Action":"disable" ]),
		sprintf("%s=\"\"", prev_dialog_prop)),
	WixNode("Condition", ([ "Action":"enable" ]),
		sprintf("%s<>\"\"", prev_dialog_prop)),
      });
    }
  }

  class NextButton
  {
    inherit Button;
    int x = 300;
    string text = "&Next >";
    string id = "Next";
    int disabled = 1;

    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      if (!disabled) {
	res->Default = "yes";
      }
      return res;
    }

    array(WixNode) get_children()
    {
      string next_dialog_prop = sprintf("%s_NextArgs", Dialog::id);
      if (disabled) {
	return ::get_children();
      }
      return ({
	@::get_children(),
	WixNode("Publish", ([
		  "Event":"NewDialog",
		  "Value":sprintf("[%s]", next_dialog_prop),
		]),
		sprintf("%s<>\"\"", next_dialog_prop)),
	WixNode("Publish", ([
		  "Event":"EndDialog",
		  "Value":"Return",
		]),
		sprintf("%s=\"\"", next_dialog_prop)),
      });
    }
  }

  class ProgressBar
  {
    inherit Control;
    constant type = "ProgressBar";
    string id = "ProgressBar";
    int x = 18;
    int y = 108;
    int width = 336;
    int height = 15;
    string text = "MsiProgressBar";
    array(WixNode) get_children()
    {
      return ({
	@::get_children(),
	@map(({ "StopServices", "DeleteServices", "StartServices",
		"WriteRegistryValues", "RemoveRegistryValues",
		"RemoveFiles", "MoveFiles", "UnmoveFiles", "InstallFiles",
	       "WriteIniValues", "InstallAdminPackage", "SetProgress",
		}), lambda(string event) {
		      return WixNode("Subscribe", ([
				       "Attribute":"Progress",
				       "Event":event,
				     ]));
		    }),
      });
    }    
  }

  int width = 373;
  int height = 287;
  string title = "[ProductName]";

  array(Control) controls;

  mapping(string:string) get_attrs()
  {
    return ([
      "Id":id,
      "Width":(string)width,
      "Height":(string)height,
      "Title":title,
    ]);
  }

  WixNode gen_xml()
  {
    WixNode res = WixNode("Dialog", get_attrs());
    foreach(controls, Control c) {
      res->add_child(c->gen_xml());
    }
    return res;
  }

  protected void create()
  {
    // NOTE: __INIT must have finished before the objects are cloned.
    controls = ({
      BannerBmp(),
      BannerText(),
      Line(),
      CancelButton(),
      PrevButton(),
      NextButton(),
      Line2(),
    });
  }
}

class ExitDialog
{
  inherit Dialog;

  string id = "ExitDialog";
  string banner = "Installation Interrupted";

  class NextButton
  {
    inherit Dialog::NextButton;
    string text = "&Close";
    string id = "Close";

    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      res->Cancel = "yes";
      m_delete(res, "Disabled");
      return res;
    }

    array(WixNode) get_children()
    {
      return ({
	@::get_children(),
	WixNode("Publish", ([
		  "Event":"EndDialog",
		  "Value":"Return",
		]), "1"),
      });
    }
  }
  class CancelButton
  {
    inherit Dialog::CancelButton;
    int disabled = 1;
  }
  class PrevButton
  {
    inherit Dialog::PrevButton;
    int disabled = 1;
  }
  class BodyText
  {
    inherit Dialog::BodyText;
    string text = "The installation was interrupted before [ProductName] "
      "could be installed. You need to restart the installer to try again.";
  }

  protected void create()
  {
    ::create();
    controls += ({ BodyText() });
  }
}

class FatalDialog
{
  inherit ExitDialog;
  string id = "FatalDialog";
  string banner = "Installation Incomplete";
}

class FolderDialog
{
  inherit Dialog;

  string id = "FolderForm";
  string banner = "Select Installation Folder";

  mapping(string:string) get_attrs()
  {
    mapping(string:string) res = ::get_attrs();
    res->TrackDiskSpace = "yes";
    return res;
  }

  class NextButton
  {
    inherit Dialog::NextButton;

    int disabled = 0;

    array(WixNode) get_children()
    {
      return ({
	@::get_children(),
	WixNode("Publish", ([
		  "Event":"SetTargetPath",
		  "Value":"TARGETDIR",
		]), "1"),
	WixNode("Publish", ([
		  "Event":"EndDialog",
		  "Value":"Return",
		]), "1"),
      });
    }
  }

  class FolderLabel
  {
    inherit TextLabel;
    int y = 114;
    int width = 348;
    string text = "&Folder:";
    string id = "FolderLabel";
    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      res->TabSkip = "no";
      return res;
    }
  }

  class FolderEdit
  {
    inherit Control;
    constant type = "PathEdit";
    int x = 18;
    int y = 126;
    int width = 252;
    int height = 18;
    string id = "PathEdit";
    string text = "MsiPathEdit";
    
    mapping(string:string) get_attrs()
    {
      mapping(string:string) res = ::get_attrs();
      res->Sunken = "yes";
      res->Property = "TARGETDIR";
      return res;
    }
  }

  protected void create()
  {
    ::create();
    controls += ({
      FolderLabel(),
      FolderEdit(),
    });
  }
}

class ProgressDialog
{
  inherit Dialog;

  string id = "ProgressDialog";
  string banner = "Installing...";

  class PrevButton
  {
    inherit Dialog::PrevButton;
    int disabled = 1;
  }

  mapping(string:string) get_attrs()
  {
    mapping(string:string) res = ::get_attrs();
    res->Modeless = "yes";
    return res;
  }

  protected void create()
  {
    ::create();
    controls += ({
      ProgressBar(),
    });
  }
}

class UI
{
  array(Dialog) dialogs = ({
    ExitDialog(),
    FatalDialog(),
    FolderDialog(),
    ProgressDialog(),
  });

  WixNode gen_xml()
  {
    WixNode res = WixNode("UI", ([]))->
      add_child(WixNode("Property", ([ "Id":"DefaultUIFont" ]),
			"VsdDefaultUIFont.524F4245_5254_5341_4C45_534153783400"));
    //res->add_child(WixNode("Property", ([ "Id":"ErrorDialog" ]), "ErrorDialog"));
    foreach(dialogs, Dialog d) {
      res->add_child(d->gen_xml());
    }
    res->add_child(WixNode("TextStyle", ([
			     "Id":"VSI_MS_Sans_Serif13.0_0_0",
			     "FaceName":"MS Sans Serif",
			     "Size":"9",
			     "Red":"0",
			     "Green":"0",
			     "Blue":"0",
			   ])));
    res->add_child(WixNode("TextStyle", ([
			     "Id":"VSI_MS_Sans_Serif16.0_1_0",
			     "FaceName":"MS Sans Serif",
			     "Size":"12",
			     "Red":"0",
			     "Green":"0",
			     "Blue":"0",
			     "Bold":"yes",
			   ])));
    res->add_child(WixNode("TextStyle", ([
			     "Id":"VsdDefaultUIFont.524F4245_5254_5341_4C45_534153783400",
			     "FaceName":"MS Sans Serif",
			     "Size":"9",
			     "Red":"0",
			     "Green":"0",
			     "Blue":"0",
			   ])));
    res->add_child(WixNode("InstallUISequence", ([]))->
		   add_child(WixNode("Show", ([
				       "Dialog":"FatalDialog",
				       "OnExit":"error",
				     ]),
				     "NOT HideFatalErrorForm"))->
		   add_child(WixNode("Custom", ([
				       "After":"ValidateProductID",
				       "Action":"InitTarget",
				     ]),
				     "TARGETDIR=\"\""))->
		   add_child(WixNode("Show", ([
				       "After":"CostFinalize",
				       "Dialog":"FolderForm",
				     ])))->
		   add_child(WixNode("Show", ([
				       "After":"FolderForm",
				       "Dialog":"ProgressDialog",
				     ]))));
    return res;
  }
}

#endif /* GENERATE_WIX_UI */

array(string) get_subdirs (string dir)
// Returns a listing of all directories in the tree below dir.
{
  array(string) dirs = ({dir});
  for (int pos = 0; pos < sizeof (dirs); pos++) {
    foreach (get_dir (dirs[pos]), string entry) {
      entry = combine_path (dirs[pos], entry);
      if (Stdio.is_dir (entry))
	dirs += ({entry});
    }
  }
  return sort (dirs[1..]);
}

int compare_files(string a,string b)
{
  mixed sa=file_stat(a);
  mixed sb=file_stat(b);
  if(sa && sb && sa[1]==sb[1])
    return Stdio.read_file(a) == Stdio.read_file(b);
  return 0;
}

int compare_to_file(string data,string a)
{
  mixed sa=file_stat(a);
  if(sa && sa[1]==sizeof(data))
    return Stdio.read_file(a) == data;
  return 0;
}

string stripslash(string s)
{
  while(sizeof(s)>1 && s[-1]=='/') s=s[..sizeof(s)-2];
  return s;
}


mapping vars=([]);

object reg;

string regquote(string s)
{
  while(s[-1] == '/' || s[-1]=='\\') s=s[..sizeof(s)-2];
  return
    replace(s,
	    ({".","[","]","*","\\","(",")","|","+"}),
	    ({"\\.","\\[","\\]","\\*","\\\\","\\(","\\)","\\|","\\+"}) );
}

string globify(string s)
{
  if(s[-1]=='/') s=s[..sizeof(s)-2];
  return s+"*";
}

string fakeroot(string s)
{
  if(!vars->fakeroot) return s;
  if(!reg)
  {
    reg=Regexp(sprintf("^([^/])%{|(%s)%}",
		       Array.map(
				 ({
				   getcwd(),
				   vars->LIBDIR_SRC,
				   vars->SRCDIR,
				   vars->TMP_BINDIR,
				   vars->BASEDIR,
				   vars->MANDIR_SRC,
				   vars->DOCDIR_SRC,
				   vars->TMP_LIBDIR,
				   vars->fakeroot,
				 }) - ({ 0 }),regquote)));
  }
  if(reg->match(s)) return s;
  return vars->fakeroot+s;
}

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

#ifdef CROSS_INSTALL
string source_version;
string version()
{
  if (!source_version) {
    mapping(string:int) defs = ([]);
    Stdio.File f = Stdio.File(combine_path(vars->SRCDIR, "version.h"));
    foreach(f->line_iterator();; string line) {
      string def;
      int n;
      if (2 == sscanf(line, "#define %s %d", def, n))
	defs[def] = n;
    }
    source_version = sprintf("Pike v%d.%d release %d",
			     defs["PIKE_MAJOR_VERSION"],
			     defs["PIKE_MINOR_VERSION"],
			     defs["PIKE_BUILD_VERSION"]);
  }
  return source_version;
}
#endif

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
  array(string) split = filename / "/";
  string rest = "";
  for (int i = sizeof (split) - 1; i >= 0; i--) {
    string d = split[..i] * "/";
    if (string r = translator[d]) {
      r = combine_path (r, rest);
      return r[..sizeof (r) - 2];
    }
    rest = split[i] + "/" + rest;
  }
  werror ("Warning: Didn't translate %O\n", filename);
  return filename;
}

void tarfilter(string filename)
{
  ((program)combine_path(__FILE__, "..", "tarfilter"))()->
    main(3, ({ "tarfilter", filename, filename }));
}

#ifdef __NT__
constant tmpdir="~piketmp";
#endif /* __NT__ */

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
  error_msg ("Installation aborted.\n");
  exit(1);
}

void update_entry2()
{
  entry2->set_text( combine_path( entry1 -> get_text(), "bin/pike") );
}

#ifdef USE_GTK2
void close_fileselector(object button, object selector)
#else
void close_fileselector(object selector, object button)
#endif
{
  selector->hide();
  destruct(selector);  
}

#ifdef USE_GTK2
void set_filename(object button, array ob)
#else
void set_filename(array ob, object button)
#endif
{
  object selector=ob[0];
  object entry=ob[1];
  entry->set_text(selector->get_filename());
  if(entry == entry1)
    update_entry2();
#ifdef USE_GTK2
  close_fileselector(button,selector);
#else
  close_fileselector(selector,button);
#endif
}

#ifdef USE_GTK2
void selectfile(object button, object entry)
#else
void selectfile(object entry, object button)
#endif
{
  object selector;
  selector=GTK.FileSelection("Pike installation prefix");
  selector->set_filename(entry->get_text());
  selector->ok_button()->signal_connect("clicked", set_filename,
					({ selector, entry }) );
  selector->cancel_button()->signal_connect("clicked",close_fileselector,selector);
  selector->show();
}

void do_exit()
{
  exit(0);
}

void cancel()
{
  error_msg ("See you another time!\n");
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
  install_type="--new-style";

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

void begin_wizard(array(string) argv, string prefix)
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
#ifdef USE_GTK2
		     ->PS(GTK.Image(combine_path(vars->SRCDIR, "install-welcome")),0,0,0)
#else
		     ->PS(GTK.Pixmap(GTK.Util.load_image(combine_path(vars->SRCDIR,
								      "install-welcome"))->img),0,0,0)
#endif
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
		   combine_path(vars->exec_prefix||combine_path(prefix, "bin"),
				"pike"));

  entry1->signal_connect("focus_out_event",update_entry2,0);
  button1->signal_connect("pressed",cancel,0);
  button2->signal_connect("pressed",next,0);
  button4->signal_connect("pressed",selectfile,entry2);
  button5->signal_connect("pressed",selectfile,entry1);

  window1->show_all();
}
#endif

// Recalculate adhoc signature hashes on macOS after updating the master cookie
array(int) find_macho_signature(Stdio.File f)
{
  int magic;
  string endian;
  f->seek(0);
  sscanf(f->read(4), "%4c", magic);
  if (magic == 0xfeedfacf) {
    endian = "";
  } else if (magic == 0xcffaedfe) {
    endian = "-";
  } else
    return 0;
  array(int) header = array_sscanf(f->read(4*7), ("%"+endian+"4c")*7);
  if (sizeof(header) < 7)
    return 0;
  int ncmds = header[3];
  for(int i=0; i<ncmds; i++) {
    int cmdstart = f->tell();
    array(int) command = array_sscanf(f->read(4*2), ("%"+endian+"4c")*2);
    if (sizeof(command) < 2)
      return 0;
    if (command[0] == 0x1d && command[1] >= 8) {
      array(int) sig = array_sscanf(f->read(4*2), ("%"+endian+"4c")*2);
      if (sizeof(sig) == 2)
        return sig;
    }
    f->seek(cmdstart + command[1]);
  }
  return 0;
}

int find_superblob_code_directory(Stdio.File f, int sb_offs, int sb_len)
{
  if (sb_len < 4*3)
    return 0;
  f->seek(sb_offs);
  array(int) header = array_sscanf(f->read(4*3), "%4c"*3);
  if (sizeof(header) < 3 || header[0] != 0xfade0cc0 || header[1] > sb_len)
    return 0;
  sb_len = header[1];
  int nslot = header[2];
  if (sb_len < 4*3 + 4*2*nslot)
    return 0;
  for (int i=0; i<nslot; i++) {
    array(int) slot = array_sscanf(f->read(4*2), "%4c"*2);
    if (sizeof(slot) < 2)
      return 0;
    if (slot[0] == 0)
      return slot[1];
  }
  return 0;
}

array(int) find_code_directory_hash_layout(Stdio.File f, int sb_offs, int sb_len, int cd_offs)
{
  if (cd_offs + 40 > sb_len)
    return 0;
  f->seek(sb_offs + cd_offs);
  array(int) header = array_sscanf(f->read(40), "%4c"*9 + "%1c"*4);
  if (sizeof(header) < 13 || header[0] != 0xfade0c02 ||
      header[1] < 40 || cd_offs + header[1] > sb_len)
    return 0;
  int hash_offs = header[4];
  int n_code_slots = header[7];
  int code_limit = header[8];
  int hash_size = header[9];
  int hash_type = header[10];
  int page_size = header[12];
  if (hash_offs + n_code_slots * hash_size > sb_len)
    return 0;
  return ({ sb_offs + cd_offs + hash_offs, hash_type, hash_size,
            n_code_slots, page_size, code_limit });
}

void update_signature_hashes(Stdio.File f, int hash_offs, int hash_type,
                             int hash_size, int n_code_slots, int page_size,
                             int code_limit, int|void offset, int|void length)
{
  Crypto.Hash hash;
  switch(hash_type) {
#if constant(Crypto.SHA256)
  case 2:
    hash = Crypto.SHA256;
    break;
#endif
  default:
    error("Hash type %d not supported!\n", hash_type);
  }
  if (hash_size != hash()->digest_size())
    error("Error wrong digest size %d (!= %d) for hash type %d\n",
          hash_size, hash()->digest_size(), hash_type);
  f->seek(hash_offs);
  array(string) hashes = f->read(n_code_slots * hash_size) / hash_size;
  if (sizeof(hashes) < n_code_slots)
    n_code_slots = sizeof(hashes);
  int pos = 0;
  for (int i=0; i<n_code_slots; i++) {
    string data = "";
    if (pos < code_limit) {
      int sz = 1 << page_size;
      if (pos + sz > code_limit)
        sz = code_limit - pos;
      if (length &&
          (pos + sz <= offset || pos >= offset + length)) {
        pos += sz;
        continue;
      }
      f->seek(pos);
      data = f->read(sz);
      pos += sizeof(data);
      if (sizeof(data) < sz)
        code_limit = pos;
    }
    hashes[i] = hash()->update(data)->digest();
  }
  f->seek(hash_offs);
  f->write(hashes * "");
}

void fix_macos_adhoc_signature(Stdio.File f, int|void offset, int|void length)
{
  array(int) macho_sig = find_macho_signature(f);
  if (macho_sig) {
    int sb_code_dir = find_superblob_code_directory(f, @macho_sig);
    if (sb_code_dir) {
      array(int) hash_layout = find_code_directory_hash_layout(f, @macho_sig,
                                                               sb_code_dir);
      if (hash_layout)
        update_signature_hashes(f, @hash_layout, offset, length);
    }
  }
}


int traditional;
string exec_prefix;
string lib_prefix;
string include_prefix;
string doc_prefix;
string man_prefix;
string cflags;
string ldflags;
string lnk;
string old_exec_prefix;
object interactive;
string install_type="--interactive";

class InstallHandler(mapping vars, string prefix) {

  protected mapping already_created=([]);
  protected array(string) files_to_delete=({});
  protected array(string) files_to_not_delete=({});
  protected array(string) to_dump=({});

  protected void create()
  {
    setup_paths();
  }

  protected void setup_paths()
  {
    exec_prefix = combine_path(prefix, "bin");
    lib_prefix = combine_path(prefix, "lib");
    include_prefix = combine_path(prefix,"include","pike");
    if (!vars->TMP_BUILDDIR) vars->TMP_BUILDDIR="bin";
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

    error_msg("%s: %s\n",sprintf(fmt,@args),some_strerror(err));
    werror("Current directory = %s\n",getcwd());
    werror("**Installation failed..\n");
    exit(1);
  }

  int mkdirhier(string orig_dir)
  {
    int tomove;
    string dir=orig_dir;
    if(already_created[orig_dir]) return 1;

    if(dir=="" || (sizeof(dir)==2 && dir[-1]==':')) return 1;
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
	error_msg ("Warning: Directory '%s' already exists as a file.\n",dir);
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

  void create_file(string dest, string content)
  {
    status("Creating", dest);
    if (compare_to_file(content, dest)) {
      status("Creating", dest, "Already created");
      return;
    }
    Stdio.write_file(dest, content);
    if (status == global::status_impl)
      status("Creating", dest, "done");
  }

  int low_install_file(string from,
		       string to,
		       void|int mode,
		       void|string id)
  {
    installed_files++;

    to=fakeroot(to);

    status("Installing",to);

    if(compare_files(from,to))
    {
      status("Installing",to,"Already installed");
      return 0;
    }
    mkdirhier(dirname(to));
    if(!mode) {
      Stdio.Stat st = file_stat(from);
      if(!st)
	exit(1, "Could not find file %O\n", from);
      int src_mode = st->mode;
      if (src_mode & 0111) {
	// Executable.
	mode = 0755;
      } else {
	mode = 0644;
      }
    }

    string tmpfile=to+"-"+getpid()+"-"+time();
    if(!Stdio.cp(from,tmpfile))
      fail("copy(%s,%s)",from,tmpfile);

    // Chown and chgrp not implemented yet
    chmod(tmpfile,mode);

    // Need to rename the old file to .old
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
    rm(to+".old"); // Ignore errors

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

  // Install the file if it exists, but don't complain if it doesn't.
  int try_install_file(string from,
		       string to,
		       void|int mode,
		       void|int dump)
  {
    if(file_stat(from)) {
      if(query_num_arg()==2)
	return install_file(from, to);
      else
	return install_file(from, to, mode, dump);
    }
    return 0;
  }

  void install_dir(string from, string to, int dump)
  {
    from=stripslash(from);
    to=stripslash(to);
    
    installed_files++;
    mkdirhier(to);
    foreach(get_dir(from),string file)
    {
      if(file=="testsuite.in") continue;
      if(file[..1]==".#") continue;
      if(file[0]=='#' && file[-1]=='#') continue;
      if(file[-1]=='~') continue;
      if(has_suffix(file, ".test")) continue;
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

  // Install the file if it exists, but don't complain if it doesn't.
  void try_install_dir(string from,
		       string to,
		       int dump)
  {
    if(file_stat(from))
      install_dir(from, to, dump);
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

  // Create a master.pike with the correct lib_prefix
  void make_master(string dest, string master, string lib_prefix,
		   string include_prefix, string|void share_prefix,
		   string|void cflags, string|void ldflags)
  {
    status("Finalizing",master);
    string master_data=Stdio.read_file(master);
    if (!master_data) {
      error("Failed to read master template file %O\n", master);
    }
    master_data=replace(master_data, ({
			  "#lib_prefix#",
			  "#include_prefix#",
			  "#share_prefix#",
			  "#doc_prefix#",
			  "#cflags#",
			  "#ldflags#",
			}), ({
			  replace(lib_prefix,"\\","\\\\"),
			  replace(include_prefix,"\\","\\\\"),
			  replace(share_prefix||"#share_prefix#", "\\", "\\\\"),
			  replace(doc_prefix||"#doc_prefix#", "\\", "\\\\"),
			  replace(cflags||"", "\\", "\\\\"),
			  replace(ldflags||"", "\\", "\\\\"),
			}));
    if((vars->PIKE_MODULE_RELOC||"") != "")
      master_data = replace(master_data, "#undef PIKE_MODULE_RELOC",
			    "#define PIKE_MODULE_RELOC 1");
    if(compare_to_file(master_data, dest)) {
      status("Finalizing",dest,"- already finalized");
      return;
    }
    Stdio.write_file(dest,master_data);
    status("Finalizing",master,"done");
  }

  // Install file while fixing CC= and CXX= w.r.t. smartlink
  void fix_smartlink(string src, string dest, string include_prefix)
  {
    status("Finalizing",src);
    string data=Stdio.read_file(src);
    data = map(data/"\n", lambda(string s) {
      string cc;
      if(2==sscanf(s, "CC=%*s/smartlink %s", cc))
	return "CC="+include_prefix+"/smartlink "+cc;
      else if(2==sscanf(s, "CXX=%*s/smartlink %s", string cxx))
	return "CXX="+include_prefix+"/smartlink "+cxx;
      else
	return s;
    })*"\n";
    if(compare_to_file(data, dest)) {
      status("Finalizing",dest,"- already finalized");
      return;
    }
    Stdio.write_file(fakeroot(dest),data);
    status("Finalizing",fakeroot(dest),"done");
  }

  // dump modules (and master)
  void dump_modules()
  {
    string master=combine_path(lib_prefix,"master.pike");
    Stdio.Stat s1=file_stat(master);
    Stdio.Stat s2=file_stat(master+".o");
    mapping(string:mapping(string:string)) options = ([
      "env":getenv()-([
	"PIKE_PROGRAM_PATH":"",
	"PIKE_MODULE_PATH":"",
	"PIKE_INCLUDE_PATH":"",
	"PIKE_MASTER":"",
      ]) ]);


    if(!s2 || s1->mtime>=s2->mtime || redump_all)
    {
      int retcode;
      mixed error = catch {
	  if(file_stat(fakeroot(pike))) {
	    object p=
	      Process.create_process( ({fakeroot(pike),"-m",
					combine_path(vars->SRCDIR,"dumpmaster.pike"),
					@(vars->fakeroot?({"--fakeroot="+vars->fakeroot}):({})),
					master}), options);
	    retcode=p->wait();
	  }
	  else
	    error_msg ("Pike binary %O could not be found.\n"
		       "Dumping of master.pike failed (not fatal).\n",
		       fakeroot(pike));
	};
      if(error)
	error_msg ("Dumping of master.pike failed (not fatal)\n%s\n",
		   describe_backtrace(error));
      if(retcode)
	error_msg ("Dumping of master.pike failed (not fatal) (0x%:08x)\n",
		   retcode);
    }

    if(!sizeof(to_dump)) return;

    rm("dumpmodule.log");

    foreach(to_dump, string mod)
      if (file_stat(mod+".o"))
	rm(mod+".o");

    array cmd=({ fakeroot(pike) });

    if(vars->fakeroot)
      cmd+=({
	sprintf("-DPIKE_FAKEROOT=%O",vars->fakeroot),
	sprintf("-DPIKE_FAKEROOT_OMIT=%O",
		map( ({
		       getcwd(),
		       vars->LIBDIR_SRC,
		       vars->SRCDIR,
		       vars->TMP_BINDIR,
		       vars->MANDIR_SRC,
		       vars->DOCDIR_SRC,
		       vars->TMP_LIBDIR,
		       vars->BASEDIR,
		       vars->fakeroot,
		     }) - ({ 0 }), globify)*":"),
	"-m",combine_path(vars->TMP_LIBDIR,"master.pike")
      });

    cmd+=({ "-x", "dump",
	    "--log-file",	// --distquiet below might override this.
#ifdef USE_GTK
	    label1?"--distquiet":
#endif
	    (verbose ? "--verbose" : "--quiet")});

    // Dump 25 modules at a time as to not confuse systems with
    // very short memory for application arguments.

    int offset = 1;
  dumploop:
    foreach(to_dump/25.0, array delta_dump)
    {
      mixed err = catch {
	  object p=
	    Process.create_process(cmd +
				   ( istty() ?
				     ({
				       sprintf("--progress-bar=%d,%d",
					       offset, sizeof(to_dump))
				     }) : ({}) ) +
				   delta_dump, options);
	  int retcode=p->wait();
	  if (retcode) {
	    error_msg("Dumping of some modules failed (not fatal) (0x%:08x):\n"
		      "%{  %O\n%}",
		      retcode, delta_dump);
	    break dumploop;
	  }
	};
      if (err) {
	error_msg ("Failed to spawn module dumper (not fatal):\n"
		   "%s\n", describe_backtrace(err));
	break;
      }

      offset += sizeof(delta_dump);
    }

    if(progress_bar)
      // The last files copied do not really count (should
      // really be a third phase)...
      progress_bar->set_phase(1.0, 0.0);

    status_clear(1);
  }

  protected void create_progress_bar()
  {
    status1("Installing Pike in %s, please wait...\n", fakeroot(prefix));
    catch {
      files_to_install = (int)Stdio.read_file
	(combine_path(vars->TMP_BUILDDIR, "num_files_to_install"));

      if(files_to_install)
	progress_bar =
	  Tools.Install.ProgressBar("Installing", 0,
				    files_to_install, 0.0, 0.2);
    };
  }

#ifdef __NT__
  protected void export_and_install_dll_and_pdb()
  {
    // Export and install needed dll files (like libmySQL.dll if
    // available) that have been copied to the build dir.
    foreach(glob("*.dll", get_dir(vars->TMP_BUILDDIR)), string dll_name)
      install_file(combine_path(vars->TMP_BUILDDIR, dll_name),
		   combine_path(exec_prefix, dll_name));
	
    // Copy the Program Database (debuginfo)
    if(file_stat(combine_path(vars->TMP_BUILDDIR, "pike.pdb")))
      install_file(combine_path(vars->TMP_BUILDDIR, "pike.pdb"),
		   combine_path(exec_prefix, "pike.pdb"));
  } 

  protected void install_dlls_and_crt()
  {
#ifdef PRIVATE_CRT
    // Copy the manifests at install time.
    foreach (glob ("*.manifest", get_dir (vars->TMP_BUILDDIR)), string file)
      install_file (combine_path (vars->TMP_BUILDDIR, file),
		    combine_path (exec_prefix, file));
#endif	// PRIVATE_CRT

    export_and_install_dll_and_pdb();
  }
#endif

  protected void do_install_libpike()
  {
    low_install_file("pike.so", combine_path(vars->TMP_LIBDIR, "pike.so"));
  }

  protected void do_install_master(string master_src)
  {
    make_master(combine_path(vars->TMP_LIBDIR,"master.pike"), master_src,
		lib_prefix, include_prefix, UNDEFINED, cflags, ldflags);
  }

  protected void do_install_aux()
  {
  }

  void do_install()
  {
    create_progress_bar();
    
    mixed err = catch {

	finalize_pike();
#ifdef __NT__
	install_dlls_and_crt();
#endif

	install_file(combine_path(vars->TMP_BUILDDIR,"pike.syms"),
		     pike+".syms");
	
	// Support installation in LIBPIKE mode.
	if (file_stat("pike.so")) {
	  do_install_libpike();
	}

	do_install_master(combine_path(vars->LIBDIR_SRC,"master.pike.in"));
	do_install_aux();
	
	install_dir(vars->TMP_LIBDIR,lib_prefix,1);
	install_dir(vars->LIBDIR_SRC,lib_prefix,1);

	install_header_files(vars->SRCDIR,include_prefix);
	install_header_files(combine_path(vars->SRCDIR,"code"),
			     combine_path(include_prefix,"code"));
	install_header_files(vars->TMP_BUILDDIR,include_prefix);

	install_file(combine_path(vars->SRCDIR,"make_variables.in"),
		     combine_path(include_prefix,"make_variables.in"));
	install_file(combine_path(vars->SRCDIR,"aclocal.m4"),
		     combine_path(include_prefix,"aclocal.m4"));
	install_file(combine_path(vars->SRCDIR,"run_autoconfig"),
		     combine_path(include_prefix,"run_autoconfig"));
	install_file(combine_path(vars->SRCDIR,"precompile2.sh"),
		     combine_path(include_prefix,"precompile.sh"), 0755);

	if (!no_autodoc) {
	  // install the core extracted autodocs
	  try_install_file(combine_path(vars->TMP_BUILDDIR, "autodoc.xml"),
			   combine_path(doc_prefix, "src", "core_autodoc.xml"));
	  try_install_dir(combine_path(vars->TMP_BUILDDIR, "doc_build", "images"),
			  combine_path(doc_prefix, "src", "images"), 0);
	}
	else {
	  mkdirhier(combine_path(doc_prefix, "src", "images"));
	}
	// create a directory for extracted module documentation
	mkdirhier(combine_path(doc_prefix, "src", "extracted"));

	try_install_dir(combine_path(vars->DOCDIR_SRC, "presentation"),
			combine_path(doc_prefix, "src", "presentation"), 0);
	try_install_dir(combine_path(vars->DOCDIR_SRC, "src_images"),
			combine_path(doc_prefix, "src", "src_images"), 0);
	try_install_dir(combine_path(vars->DOCDIR_SRC, "structure"),
			combine_path(doc_prefix, "src", "structure"), 0);
	try_install_dir(combine_path(vars->DOCDIR_SRC, "chapters"),
			combine_path(doc_prefix, "src", "chapters"), 0);

	foreach(({"Makefile", "doxfilter.sh", "doxygen.cfg", "inlining.txt",
		  "keywords.txt", "syntax.txt", "tags.txt", "template.xsl",
		  "xml.txt", }), string f)
	  install_file(combine_path(vars->DOCDIR_SRC, f),
		       combine_path(doc_prefix, "src", f));

	foreach(({"install_module", "smartlink",
		  "fixdepends.sh", "mktestsuite", "test_pike.pike"}), string f)
	  install_file(combine_path(vars->TMP_BINDIR,f),
		       combine_path(include_prefix,f));

	install_file(combine_path(vars->TMP_BINDIR,"precompile_installed.pike"),
		     combine_path(include_prefix,"precompile.pike"));

	mkdirhier(combine_path(include_prefix, "modules"));
	fix_smartlink(combine_path(vars->TMP_BUILDDIR,
				   "modules/dynamic_module_makefile"),
		      combine_path(include_prefix,
				   "modules/dynamic_module_makefile"),
		      include_prefix);
	fix_smartlink(combine_path(vars->TMP_BUILDDIR,
				   "propagated_variables"),
		      combine_path(include_prefix, 
				   "propagated_variables"),
		      include_prefix);
	fix_smartlink(combine_path(vars->TMP_BUILDDIR,"specs"),
		      combine_path(include_prefix,"specs"), include_prefix);

	if(file_stat(vars->MANDIR_SRC))
	  {
	    install_dir(vars->MANDIR_SRC,combine_path(man_prefix,"man1"),0);
	  }
      };

    status_clear();

    if(err) throw(err);

    catch {
      Stdio.write_file(combine_path(vars->TMP_BUILDDIR,"num_files_to_install"),
		       sprintf("%d\n",installed_files));
      write_num_files_to_export();
    };

    files_to_install=0;

    do_post_install_actions();
  }

  protected void write_num_files_to_export()
  {
  }

  protected void do_post_install_actions()
  {
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
	  error_msg ("Failed to move %s\n",lnk);
	  exit(1);
	}
      }
      if (old_exec_prefix) {
	mkdirhier(fakeroot(old_exec_prefix));
      }
      mkdirhier(fakeroot(dirname(lnk)));
      symlink(pike,fakeroot(lnk));
      catch {
	rm(fakeroot(lnk)+__MAJOR__+__MINOR__);
	symlink(pike,fakeroot(lnk)+__MAJOR__+__MINOR__);
      };
      status("Creating",lnk,"done");
    }
#endif

    progress_bar = 0;
    status1("Pike installation completed successfully.");
  }

  void finalize_pike()
  {
    pike=combine_path(exec_prefix,"pike");

    // Ugly way to detect NT installation
    string pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike");
    string suffix = "";
    if(file_stat(pike_bin_file+".exe"))
    {
      pike_bin_file+=".exe";
      pike+=".exe";
      suffix = ".exe";
    }

    low_finalize_pike(pike_bin_file, suffix);
  }

  void low_finalize_pike(string pike_bin_file, string suffix)
  {
    status("Finalizing",pike_bin_file);
    string pike_bin=Stdio.read_file(pike_bin_file);

    if (!pike_bin) {
      // Failed to read bin file, most likely Cygwin.

      status("Finalizing",pike_bin_file,"FAILED");
      if (!istty()) {
	error_msg ("Finalizing of %O failed!\n", pike_bin_file);
	error_msg ("Not found in %s.\n"
		   "%O\n", getcwd(), get_dir("."));
	error_msg ("BUILDDIR: %O\n"
		   "exe-stat: %O\n",
		   vars->TMP_BUILDDIR, file_stat(pike_bin_file+".exe"));
      }
      exit(1);
    }

    int pos=search(pike_bin, MASTER_COOKIE);

    if(pos>=0)
    {
      status("Finalizing",pike_bin_file,"...");
      pike_bin_file=combine_path(vars->TMP_BUILDDIR,"pike.tmp");
      Stdio.write_file(pike_bin_file, pike_bin);
      Stdio.File f=Stdio.File(pike_bin_file,"rw");
      int master_pos = pos+sizeof(MASTER_COOKIE);
      string master_value = combine_path(lib_prefix,"master.pike");
      f->seek(master_pos);
      f->write(master_value);
      fix_macos_adhoc_signature(f, master_pos, sizeof(master_value));
      f->close();
      status("Finalizing",pike_bin_file,"done");
      if(install_file(pike_bin_file,pike,0755)) {
	redump_all=1;
      }
      rm(pike_bin_file);
    }
    else {
      error_msg ("Warning! Failed to finalize master location!\n");
      if(install_file(pike_bin_file,pike,0755)) {
	redump_all=1;
      }
    }
  }
}

class FakerootSetupHandler {
  inherit InstallHandler;

  protected void create(mapping vars, string tmp)
  {
    this::vars = vars;

    if(tmp[-1]=='/' || tmp[-1]=='\\')
      tmp=tmp[..sizeof(tmp)-2];

    // Create the fakeroot if it doesn't exist
    // This must be done with fakeroot unset since
    // it would create fakeroot/fakeroot otherwise
    mkdirhier(tmp);
    vars->fakeroot=tmp;
  }
}

class SimpleStatus
{
  protected void status1_impl(string fmt, mixed ... args)
  {
    write(fmt+"\n", @args);
  }

  protected void status_impl(string|void doing, void|string file, string|void msg)
  {
    if(!file) file="";

    if(msg) file+=" "+msg;
    if(doing) file=doing+": "+file;
    if (file != "") write (file + "\n");
  }

  protected void set_simple_status(int simple)
  {
    status1 = (simple? status1_impl : global::status1_impl);
    status = (simple? status_impl : global::status_impl);
  }
  
  protected void create()
  {
    set_simple_status(1);
  }
}  


#ifdef SUPPORT_WIX
class WixInstallHandler {
  inherit SimpleStatus;
  inherit InstallHandler;

  protected void create()
  {
    SimpleStatus::create();
  }
  
  protected string add_msm (Directory root, string msm_glob, string descr,
			    void|string targetdir, void|string language)
  {
    if (string msm_dir = getenv ("CRT_MSM_PATH")) {
      string msm_file;

      if (Stdio.is_dir (msm_dir)) {
	array(string) all_files = get_dir (msm_dir);
	array(string) files = ({});
	foreach (all_files, string file)
	  if (glob (msm_glob, lower_case (file)))
	    files += ({file});
	switch (sizeof (files)) {
	case 1:
	  msm_file = files[0];
	  break;
	case 2..:
	  error_msg ("Warning: More than one msm for %s found:\n"
		     "%{  %s\n%}",
		     descr, map (files,
				 lambda (string file) {
				   return combine_path (msm_dir, file);
				 }));
	  return 0;
	}
      }

      if (!msm_file) {
	error_msg ("Warning: No file found matching %s - "
		   "the msm for %s won't be included.\n",
		   combine_path (msm_dir, msm_glob), descr);
	return 0;
      }

      string id =
	has_suffix (lower_case (msm_file), ".msm") ?
	msm_file[..sizeof (msm_file) - 5] : msm_file;
      root->merge_module (".", combine_path (msm_dir, msm_file),
			  id, targetdir || "TARGETDIR", language);
      status ("Adding merge module", combine_path (msm_dir, msm_file));
      return id;
    }

    else {
      error_msg ("Warning: CRT_MSM_PATH not set - can't find msm for %s.\n",
		 descr);
      return 0;
    }
  }

  // Create a versioned root wix file that installs Pike_module.msm.
  void make_wix()
  {
    Directory root = Directory("SourceDir",
			       Standards.UUID.UUID(version_guid)->encode(),
			       "TARGETDIR");
    /* Workaround for bug in light. */
    root->extra_ids["PIKE_TARGETDIR"] = 1;

    // Note: TARGETDIR and PIKE_TARGETDIR are always the same dir here.

    root->merge_module(".", "Pike_module.msm", "Pike", "TARGETDIR");

    // FIXME: Use the proper WIX method for this.
    // FIXME: Add an installer action that refreshes the icons in the
    // shell (call SHChangeNotify somehow?)
    root->install_regkey("bin", "HKCR", ".pike", "", "pike_file",
			 "RE__PF");
    root->install_regkey("bin", "HKCR", "pike_file", "", "Pike Script File",
			 "RE__PSF");
    root->install_regkey("bin", "HKCR", "pike_file\\shell\\run", "",
			 "Run", "RE__VERB");
    root->install_regkey("bin", "HKCR", "pike_file\\shell\\run\\command", "",
			 "\"[TARGETDIR]bin\\pike.exe\" \"%1\" %*",
			 "RE__COMMAND");
    root->install_regkey("bin", "HKCR", "pike_file\\DefaultIcon", "",
			 "[TARGETDIR]bin\\pike.exe", "RE__ICON");

    root->install_regkey("bin", "HKCR", ".pmod", "", "pike_module",
			 "RE__PM");
    root->install_regkey("bin", "HKCR", "pike_module", "", "Pike Module File",
			 "RE__PMF");
    //FIXME: Should have a diffrent icon. Traditionally "pike_blue.ico".
    root->install_regkey("bin", "HKCR", "pike_module\\DefaultIcon", "",
			 "[TARGETDIR]bin\\pike.exe", "RE__PMICON");

    string title = 
#if 1
      "Pike"
#else /* !1 */
      sprintf("Pike v%d.%d release %d",
	      __REAL_MAJOR__, __REAL_MINOR__, __REAL_BUILD__)
#endif /* 1 */
      ;

    WixNode feature_node =
      WixNode("Feature", ([
		"ConfigurableDirectory":"TARGETDIR",
		"Title":title,
		"Level":"1",
		"Id":"F_Pike",
	      ]))->
      add_child(line_feed)->
      add_child(WixNode ("ComponentRef",
			 (["Id": root->get_component_id("bin")])))->
      add_child(line_feed)->
      add_child(WixNode("MergeRef", ([ "Id":"Pike" ])))->
      add_child(line_feed);

#ifndef PRIVATE_CRT
    if (include_crt) {
      // Always include the nondebug CRT since some lib dlls might be
      // using it.
      //
      // NB: This ought to be in Pike_module.wxs, but merge modules cannot
      // contain merge modules

      string crt_arch;
      mapping(string:mixed) u = uname();
      if (u->sysname && has_prefix (u->sysname, "Win"))
	// We're running on windows (presumably we're using the pike
	// which is being packaged) so we can look up the arch from
	// the uname data.
	crt_arch = (["i86pc": "x86", "amd64": "x86_x64"])[uname()->machine];
      if (!crt_arch)
	// Probably not running on windows. Use a "*" glob to pick up
	// whatever the build environment has got in $CRT_MSM_PATH.
	crt_arch = "*";

      if (string id =
	  add_msm (root, "microsoft_*_crt_" + crt_arch + ".msm",
		   "MS CRT", 0, "0"))
	feature_node->add_child (WixNode ("MergeRef", (["Id": id])))
	  ->add_child (line_feed);
      if (string id =
	  add_msm (root, "policy_*_microsoft_*_crt_" + crt_arch + ".msm",
		   "MS CRT policy", 0, "0"))
	feature_node->add_child (WixNode ("MergeRef", (["Id": id])))
	  ->add_child (line_feed);
      if (include_crt == "debug") {
	if (string id =
	    add_msm (root, "microsoft_*_debugcrt_" + crt_arch + ".msm",
		     "MS debug CRT", 0, "0"))
	  feature_node->add_child (WixNode ("MergeRef", (["Id": id])))
	    ->add_child (line_feed);
	if (string id =
	    add_msm (root, "policy_*_microsoft_*_debugcrt_" + crt_arch + ".msm",
		     "MS debug CRT policy", 0, "0"))
	  feature_node->add_child (WixNode ("MergeRef", (["Id": id])))
	    ->add_child (line_feed);
	error_msg ("Warning: MS debug CRT is included - "
		   "it is not redistributable.\n");
	error_msg (#"\
Warning: Some libs might be linked to the release CRT so you might get
an extra CRT instance.\n");
      }
    }
#endif

    // Generate the XML.
    Parser.XML.Tree.SimpleRootNode root_node =
      Parser.XML.Tree.SimpleRootNode()->
      add_child(Parser.XML.Tree.SimpleHeaderNode((["version": "1.0",
						   "encoding": "utf-8"])))->
      add_child(WixNode("Wix", (["xmlns":Standards.XML.Wix.wix_ns]))->
		add_child(line_feed)->
		add_child(WixNode("Product", ([
				    "Manufacturer":"IDA",
				    "Name":title,
				    "Language":"1033",
				    "UpgradeCode":pike_upgrade_guid,
				    "Id":version_guid,
				    "Version":version_str,
				  ]))->
			  add_child(line_feed)->
			  add_child(WixNode("Package", ([
					      "Manufacturer":"IDA",
					      "Languages":"1033",
					      "Compressed":"yes",
					      "InstallerVersion":"300",
					      "Platforms":"Intel",
					      "SummaryCodepage":"1252",
					      "Id":version_guid,
					    ])))->
			  add_child(line_feed)->
			  add_child(WixNode("Media", ([
					      "Cabinet":"Pike.cab",
					      "EmbedCab":"yes",
					      "Id":"1",
					    ])))->
			  add_child(line_feed)->
			  add_child(root->gen_xml(UNDEFINED, "1"))->
			  add_child(line_feed)->
			  add_child(feature_node)->
			  add_child(line_feed)->
			  add_child(WixNode("AdminExecuteSequence", ([]), "\n")->
				    add_child(WixNode("Custom", ([
							"Before":"CostInitialize",
							"Action":"DIRCA_TARGETDIR",
						      ]), "TARGETDIR=\"\""))->
				    add_child(line_feed))->
			  add_child(line_feed)->
			  add_child(WixNode("InstallExecuteSequence", ([
					    ]), "\n")->
				    add_child(WixNode("Custom", ([
							"Before":"ValidateProductID",
							"Action":"DIRCA_TARGETDIR",
						      ]), "TARGETDIR=\"\""))->
				    add_child(line_feed)->
				    add_child(WixNode("RemoveExistingProducts", ([
							"After":"InstallInitialize",
						      ])))->
				    add_child(line_feed))->
			  add_child(line_feed)->
			  add_child(WixNode("FragmentRef", ([
					      "Id":"PikeUI",
					    ])))->
			  add_child(line_feed))->
		add_child (line_feed))->
      add_child(line_feed);

    create_file("Pike.wxs", root_node->render_xml());
  }
}
#endif

class TraditionalInstallHandler {
  inherit InstallHandler;

  protected void setup_paths()
  {
    exec_prefix=vars->exec_prefix||(prefix+"/bin/");
    lib_prefix=vars->lib_prefix||(prefix+"/lib/pike/");
    include_prefix =
      vars->include_prefix || combine_path(prefix,"include","pike");
    doc_prefix =
      vars->doc_prefix || combine_path(prefix, "doc", "pike");
    man_prefix=vars->man_prefix||(prefix+"/share/man/");
  }
}

class NewStyleInstallHandler {
  inherit InstallHandler;

  protected void generate_prefix()
  {
    if(!(lnk=vars->pike_name) || !sizeof(lnk)) {
      lnk = combine_path(vars->exec_prefix || combine_path(vars->prefix, "bin"),
			 "pike");
      old_exec_prefix=vars->exec_prefix; // to make the directory for pike link
    }
    prefix = combine_path("/", getcwd(), prefix, "pike",
			  replace(version()-"Pike v"," release ","."));
    exec_prefix=combine_path(prefix,"bin");
  }

  protected void setup_paths()
  {
    generate_prefix();
    lib_prefix=combine_path(prefix,"lib");
    doc_prefix=combine_path(prefix,"doc");
    include_prefix=combine_path(prefix,"include","pike");
    man_prefix=combine_path(prefix,"share/man");
  }
}

class ExportInstallHandler {
  inherit SimpleStatus;
  inherit NewStyleInstallHandler;
  protected int allow_mkdirhier = 0;
  protected string export_base_name;
  
  protected void create_export_base()
  {
  }

  protected string get_export_base_name(string ver)
  {
#if constant(uname) && !defined(CROSS_INSTALL)
    mapping(string:string) u = uname();
    string ret;
    if( u->sysname=="AIX" )
    {
      ret = sprintf("%s-%s-%s.%s",
		    ver,
		    u->sysname,
		    u->version,
		    u->release);
    }
    else {
      ret = sprintf("%s-%s-%s-%s",
		    ver,
		    u->sysname,
		    u->release,
		    u->machine);
    }
    return (replace(ret, (["/": "-", "?": ""]))
	    / " " - ({""})
	    ) * "-";
#else
    return ver;
#endif
  }
  
  protected void setup_paths()
  {
    export_base_name = get_export_base_name(replace( version(), ([ " ":"-", " release ":"." ]) ));

    status1("Building export %s", export_base_name);

    create_export_base();
    ::setup_paths();
    low_install_file(combine_path(vars->TMP_BINDIR,"install.pike"),
		     combine_path(prefix, "bin/install.pike"));
  }

  int mkdirhier(string orig_dir)
  {
    if(!allow_mkdirhier) return 1;
    ::mkdirhier(orig_dir);
  }

  void fix_smartlink(string src, string dest, string include_prefix)
  {
  }

  void export_file (string from, string tmp, string to, void|string id);
  // from: File source. tmp: Name in the build/unpack tree. Same as from
  // for files already in the build tree. to: Final destination (only
  // used in WiX mode).

  int low_install_file(string from,
		       string to,
		       void|int mode,
		       void|string id)
  {
    installed_files++;
    export_file (from, from, to, id);
    return 1;
  }

  protected void write_num_files_to_export()
  {
    low_install_file(combine_path(vars->TMP_BUILDDIR,"num_files_to_install"),
		     combine_path(prefix, "build/num_files_to_install"));
  }

  protected void do_post_install_actions()
  {
    do_export();
  }

  protected void create_progress_bar()
  {
  }

  protected void do_install_build_aux()
  {
      low_install_file(combine_path(vars->SRCDIR,"install-welcome"),
		       combine_path(prefix, "build/install-welcome"));
      low_install_file(combine_path(vars->SRCDIR,"dumpmaster.pike"),
		       combine_path(prefix, "build/dumpmaster.pike"));
  }
  
  protected void do_install_aux()
  {
    low_install_file(combine_path(vars->TMP_BUILDDIR,"specs"),
		     combine_path(include_prefix, "specs"));
    low_install_file(combine_path(vars->TMP_BUILDDIR,
				  "modules/dynamic_module_makefile"),
		     combine_path(include_prefix, 
				  "modules/dynamic_module_makefile"));
    low_install_file(combine_path(vars->TMP_BUILDDIR,
				  "propagated_variables"),
		     combine_path(include_prefix, 
				  "propagated_variables"));

    do_install_build_aux();
    
    void basefile(string x) {
      string from = combine_path(vars->BASEDIR,x);
      if(!Stdio.cp(from, x))
	error_msg ("Could not copy %s to %s.\n", from ,x);
      low_install_file(x, combine_path(prefix, x));
    };
    
    basefile("ANNOUNCE");
    basefile("COPYING");
    basefile("COPYRIGHT");
  }

  
#ifdef __NT__
  protected void check_vc8()
  {
  }

  protected void install_dlls_and_crt()
  {
    collect_dlls (lambda (string dll_path) {
      status ("Including dll", dll_path);
      string name = basename (dll_path);
      export_file (dll_path,
		   combine_path (vars->TMP_BUILDDIR, name),
		   combine_path (exec_prefix, name));
    });
    
    // $PIKE_BUILD_ROOT/install-tree contains other files that needs
    // to be added to the installation. The paths below this
    // directory will be mirrored below the pike installation
    // directory.
    if (string pike_build_root = getenv ("PIKE_BUILD_ROOT")) {
      string install_tree_path = combine_path (pike_build_root,
					       "install-tree");
      ADT.Queue install_dirs = ADT.Queue (install_tree_path);
      while (string path = install_dirs->get()) {
	foreach (get_dir (path) || ({}), string dirent) {
	  dirent = combine_path (path, dirent);
	  if (Stdio.Stat stat = file_stat (dirent)) {
	    if (stat->isdir)
	      install_dirs->put (dirent);
	    else if (stat->isreg) {
	      string install_path = dirent[sizeof (install_tree_path) + 1..];
	      export_file (dirent,
			   combine_path (vars->TMP_BUILDDIR, install_path),
			   combine_path (prefix, install_path));
	    }
	  }
	}
      }
    }

#ifndef PRIVATE_CRT
    check_vc8();
#else  // PRIVATE_CRT
    // This way of packaging the CRT dlls is currently disabled in
    // favor of the msm files. If you enable this you probably need to
    // disable the embedded MT_FIX_* stuff in configure.in.
    //
    // Using private assemblies would be nice since it enables
    // installation without admin rights, but the problem is that
    // windows only searches for a private lib in (essentially) the same
    // directory as the .exe or .dll. This means the dll's in the
    // modules directory won't find the MS CRT when it's in the bin dir.
    //
    // Putting the MS CRT in every dir where we have a dll apparently
    // won't work either since we get strange errors about duplicate
    // assemblies with the same name but different versions (the dll's
    // are really identical of course). Also, if that error is resolved
    // I'd suspect each dll would get its own runtime CRT instance then
    // so it probably wouldn't work anyway.
    if (include_crt) {
      // Find and copy dynamic CRT dlls (and manifest files as
      // required by MSVC 8.0) to the build dir.
      //
      // Note that this currently is adapted for MSVC 8.0 but it
      // could be adapted to find the runtime libraries for other
      // MSVC versions and other compilers.
      
      if (!Stdio.is_file (combine_path (vars->TMP_BUILDDIR,
					"pike.exe.manifest")))
	error_msg ("Warning: Got request to include CRT libraries "
		   "but this doesn't look like a build made by VC8 "
		   "(or something compatible).\n");
      
      else {
	string msvc_arch = getenv ("MSVC_ARCH");
	if (!msvc_arch)
	  // MSVC_ARCH should be set in the build environment. It's
	  // the same arch identifier that you give to MSVC's
	  // vcvarsall.bat to set up the build environment
	  // (vcvarsall.bat doesn't set it - you have to).
	  error_msg ("Warning: MSVC_ARCH not set - "
		     "cannot find CRT dlls to include.\n");
	      
	else
	find_crt: {
	    // The path should contain .../Microsoft Visual Studio
	    // XXX/VC/bin, so we search relative to it for the redist
	    // dir.
	    foreach (getenv ("PATH") / ";", string bin_dir) {
	      string redist_dir = combine_path (bin_dir, "../redist");
	      if (Stdio.is_dir (redist_dir)) {
		array(string) subdirs =
		  map (get_subdirs (redist_dir), lower_case);
		subdirs = glob ("*/" + msvc_arch + "/*." +
				(include_crt == "debug" ? "debug" : "") +
				"crt", subdirs);
		
		switch (sizeof (subdirs)) {
		case 1:
		  foreach (get_dir (subdirs[0]), string file)
		    export_file (combine_path (subdirs[0], file),
				 combine_path (vars->TMP_BUILDDIR, file),
				 combine_path (exec_prefix, file));
		  break find_crt;
		  
		case 2..:
		  error_msg ("Warning: More than one CRT directory found:\n"
			     "%{  %s\n%}", subdirs);
		  break find_crt;
		}
	      }
	    }
	    
	    error_msg ("Warning: Couldn't find any CRT directory "
		       "by searching PATH:\n"
		       "%{  %s\n%}", getenv ("PATH") / ";");
	  }
      }
    }
#endif	// PRIVATE_CRT
    
    export_and_install_dll_and_pdb();
  }
#endif

  void do_export();

  void low_finalize_pike(string pike_bin_file, string suffix)
  {
    low_install_file(pike_bin_file, pike, 0755, "BIN_PIKE");
  }
}

class TarExportInstallHandler {
  inherit ExportInstallHandler;
  protected array(array(string)) to_export=({});

  protected void create_export_base()
  {
    if (!mkdir(export_base_name+".dir")) {
      error("Failed to create directory %O: %s\n",
	    export_base_name+".dir", strerror(errno()));
    }

    mklink(vars->LIBDIR_SRC,export_base_name+".dir/lib");
    mklink(vars->SRCDIR,export_base_name+".dir/src");
    mklink(getcwd(),export_base_name+".dir/build");
    mklink(vars->TMP_BINDIR,export_base_name+".dir/bin");
    mklink(vars->MANDIR_SRC,export_base_name+".dir/share/man");
    mklink(vars->DOCDIR_SRC,export_base_name+".dir/refdoc");

    cd(export_base_name+".dir");

    vars->TMP_LIBDIR="build/lib";
    vars->LIBDIR_SRC="lib";
    vars->SRCDIR="src";
    vars->TMP_BINDIR="bin";
    vars->MANDIR_SRC="share/man";
    vars->DOCDIR_SRC="refdoc";
    vars->TMP_BUILDDIR="build";
  }

  void export_file (string from, string tmp, string to, void|string id)
  {
    to_export += ({({from, tmp})});
  }

  protected void do_install_libpike()
  {
    low_install_file("pike.so", combine_path(prefix, "build/pike.so"));
    ::do_install_libpike();
  }

  protected void do_install_master(string master_src)
  {
    make_master("master.pike", master_src, "build/lib", "build", "lib",
		cflags, ldflags);
    low_install_file("master.pike",
		     combine_path(prefix, "build/master.pike"));
  }

  void do_export()
  {
    allow_mkdirhier = 1;
    set_simple_status(0);

    cd("..");

    string tmpname = sprintf("PtmP%07x",random(0xfffffff));

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
#" Copyright (C) 1994-2024 IDA, Link�ping University
Pike comes with ABSOLUTELY NO WARRANTY; This is free software and you
are welcome to redistribute it under certain conditions; Read the
files COPYING and COPYRIGHT in the Pike distribution for more details.
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

    --list-files) tar xf \"$TARFILE\" "+tmpname+#".tar.gz
                  tar tfz "+tmpname+#".tar.gz
                  rm -f "+tmpname+".x "+tmpname+#".tar.gz
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
		     "  MANDIR_SRC=\\\"share/man\\\"\\\n"
		     "  DOCDIR_SRC=\\\"refdoc\\\"\\\n"
		     "  PIKE_MODULE_RELOC=\\\"" + vars->PIKE_MODULE_RELOC +
		     "\\\"\\\n"
		     "  $ARGS\"\n"
		     ")\n"
		     "rm -rf '"+export_base_name+".dir' "+tmpname+".x\n"
		     );
    chmod(tmpname+".x",0755);
    string script=sprintf("#!/bin/sh\n"
			  "tar xf \"$0\" %s.x\n"
			  "exec ./%s.x \"$0\" \"$@\"\n",
			  tmpname, tmpname);
    if(sizeof(script) >= 100)
    {
      error_msg ("Script too long!!\n");
      exit(1);
    }

    array(string) parts = script/"/";
    mkdirhier( parts[..sizeof(parts)-2]*"/");
    Stdio.write_file(script,"");

    [array(string) to_export_from, array(string) to_export_tmp] =
      Array.transpose (to_export);

    // Link in files outside the build tree during the tar.
    for (int i = 0; i < sizeof (to_export_from); i++) {
      string tmpname = to_export_tmp[i];
      to_export_tmp[i] =
	combine_path (export_base_name+".dir", to_export_tmp[i]);
      if (to_export_from[i] != tmpname) {
	rm (to_export_tmp[i]);
	mklink (to_export_from[i], to_export_tmp[i]);
      }
      else
	to_export_from[i] = 0;
    }

    string tmpmsg=".";

    string tararg="cf";
    foreach(to_export_tmp/25.0, array files_to_tar)
    {
      status("Creating", tmpname+".tar", tmpmsg);
      tmpmsg+=".";
      Process.create_process(({"tar",tararg,tmpname+".tar"})+ files_to_tar)
	->wait();
      tararg="rf";
    }

    // Clean up symlinks again.
    for (int i = 0; i < sizeof (to_export_from); i++)
      if (to_export_from[i])
	rm (to_export_tmp[i]);

    status("Filtering to root/root ownership", tmpname+".tar");
    tarfilter(tmpname+".tar");

    status("Creating", tmpname+".tar.gz");

    Process.create_process(({"gzip","-9",tmpname+".tar"}))->wait();

    status("Creating", export_base_name);

    //  Setting COPYFILE_DISABLE avoids a "._PtmP..." resource file to be added
    //  on OS X which otherwise would hinder self-extraction from bootstrapping.
    Process.create_process( ({ "tar","cf", export_base_name,
			       script, tmpname+".x", tmpname+".tar.gz" }),
			    ([ "env" : ([ "COPYFILE_DISABLE" : "true" ]) ]) )
      ->wait();

    status("Filtering to root/root ownership", export_base_name);
    tarfilter(export_base_name);

    chmod(export_base_name,0755);

    status("Cleaning up","");

    Process.create_process( ({ "rm","-rf",
			       export_base_name+".dir",
			       export_base_name+".x",
			       tmpname+".x",
			       tmpname+".tar.gz",
			       parts[0],
			    }) ) ->wait();

    status1("Export done");
  }
}

#ifdef __NT__
class BurkExportInstallHandler {
  inherit ExportInstallHandler;
  protected array(array(string)) to_export=({});

  void export_file (string from, string tmp, string to, void|string id)
  {
    to_export += ({({from, tmp})});
  }

  protected void do_install_libpike()
  {
    low_install_file("pike.so", combine_path(prefix, "build/pike.so"));
    ::do_install_libpike();
  }

  protected void do_install_master(string master_src)
  {
    string unpack_master = "master.pike";
    // We don't want to overwrite the main master...
    // This is undone by the translator.
    unpack_master = "unpack_master.pike";
    make_master(unpack_master, master_src,
		tmpdir+"/build/lib", tmpdir+"/build", tmpdir+"/lib",
		cflags, ldflags);
    low_install_file(unpack_master,
		     combine_path(prefix, "build/master.pike"));
  }

  void do_export()
  {
    status("Creating",export_base_name+".burk");
    Stdio.File p=Stdio.File(export_base_name+".burk","wc");
    string msg="   Loading installation script, please wait...";
    p->write("w%4c%s",sizeof(msg),msg);

#define TRANSLATE(X,Y) combine_path(".",X) : Y
    mapping translator = ([
      TRANSLATE(vars->BASEDIR,tmpdir),
      TRANSLATE(vars->LIBDIR_SRC,tmpdir+"/lib"),
      TRANSLATE(vars->SRCDIR,tmpdir+"/src"),
      TRANSLATE(vars->TMP_BINDIR,tmpdir+"/bin"),
      TRANSLATE(vars->MANDIR_SRC,tmpdir+"/share/man"),
      TRANSLATE(vars->DOCDIR_SRC,tmpdir+"/refdoc"),
      TRANSLATE(vars->TMP_LIBDIR,tmpdir+"/build/lib"),
      "unpack_master.pike" : tmpdir+"/build/master.pike",
      "":tmpdir+"/build",
    ]);

    [array(string) to_export_from, array(string) to_export_tmp] =
      Array.transpose (to_export);
    array(string) translated_names = Array.map(to_export_tmp, translate, translator);
    array(string) dirs=Array.uniq(Array.map(translated_names, dirname));
    while(1)
    {
      array(string) d2=Array.map(dirs, dirname) - dirs;
      if(!sizeof(d2)) break;
      dirs+=Array.uniq(d2);
    }
    dirs-=({""});
    sort(dirs);

    foreach(dirs, string dir) p->write("d%4c%s",sizeof(dir),dir);
    foreach(Array.transpose(  ({ to_export_from, translated_names }) ),
	    [ string file, string file_name ])
    {
      status("Adding",file);
      if (string f=Stdio.read_file(file)) {
	p->write("f%4c%s%4c",sizeof(file_name),file_name,sizeof(f));
	p->write(f);
      } else {
	//  Huh? File could not be found.
	error_msg ("-------------------\n"
		   "Warning: Could not add file: %s. File not found!\n"
		   "-------------------\n", file);
      }
    }

    // FIXME, support $INSTALL_SCRIPT (or similar)

#define TRVAR(X) translate(combine_path(vars->X,"."), translator)

    array(string) env=({
      //    "PIKE_MODULE_PATH="+TRVAR(TMP_LIBDIR)+"/modules:"+TRVAR(LIBDIR_SRC)+"/modules",
      //    "PIKE_PROGRAM_PATH=",
      //    "PIKE_INCLUDE_PATH="+TRVAR(LIBDIR_SRC)+"/include",
    });

    foreach(env, string e)
      p->write("e%4c%s",sizeof(e),e);

#define RELAY(X) " " #X "=" + TRVAR(X)+

    string cmd=
      replace(translate("pike.exe", translator),"/","\\")+
      " -m"+translate("unpack_master.pike", translator)+
      " -DNOT_INSTALLED" +
      " "+translate( combine_path(vars->TMP_BINDIR,"install.pike"), translator)+
      RELAY(TMP_LIBDIR)
      RELAY(LIBDIR_SRC)
      RELAY(SRCDIR)
      RELAY(TMP_BINDIR)
      RELAY(MANDIR_SRC)
      RELAY(DOCDIR_SRC)
      RELAY(BASEDIR)
      " TMP_BUILDDIR="+translate("", translator)+
      (((vars->PIKE_MODULE_RELOC||"") != "")? " PIKE_MODULE_RELOC=1":"")+
      " $" // $ = @argv
      ;

    p->write("s%4c%s",sizeof(cmd),cmd);

    array(string) to_delete=translated_names + ({translate("pike.tmp",translator)});
    to_delete=Array.uniq(to_delete);
    to_delete+=reverse(dirs);

    // Generate cleanup
    foreach(to_delete, string del)
      p->write("D%4c%s",sizeof(del),del);

    p->write("q\0\0\0\0");
    p->close("rw");

    if(last_len)
    {
      status(0,"");
      status(0,"");
    }

  }

  protected void check_vc8()
  {
    if (include_crt &&
	Stdio.is_file (combine_path (vars->TMP_BUILDDIR,
				     "pike.exe.manifest"))) {
      error_msg (#"\
Warning: Using the old exe installer with VC8 doesn't work well. Your
package probably won't work unless the user already has the MS CRTs
installed. You might want to try to enable private CRT packaging; see
the PRIVATE_CRT stuff in install.pike.\n");
    }
  }
}
#endif /* __NT__ */

#ifdef SUPPORT_WIX
class WixExportInstallHandler {
  inherit ExportInstallHandler;

  void export_file (string from, string tmp, string to, void|string id)
  {
    mapping translator = ([
      "":"",
      replace (prefix, "\\", "/"):"",
      replace (getcwd(), "\\", "/"):"",
    ]);
    root->install_file(translate(replace (to, "\\", "/"), translator),
		       from, id);
  }

  protected void do_install_master(string master_src)
  {
#if 0
    string unpack_master = "unpack_master.pike";
    make_master(unpack_master, master_src, "lib", "include/pike",
		UNDEFINED, cflags, ldflags);
    low_install_file(unpack_master,
		     combine_path(prefix, "lib/master.pike"));
#endif
    low_install_file(combine_path(vars->SRCDIR,
				  "../packaging/windows/pike.ico"),
		     combine_path(prefix, "lib/pike.ico"));
    root->uninstall_file("lib/master.pike");
  }

  protected int low_install_regkey(string path, string root, string key,
				   string name, string value, string id)
  {
    global::root->install_regkey(path, root, key, name, value, id);
  }

  protected void recurse_uninstall_file(Directory d, string pattern)
  {
    d->recurse_uninstall_file(pattern);
  }

  protected int low_uninstall_file(string path)
  {
    root->uninstall_file(path);
  }

  void do_export()
  {
    // Minimize the number of src directives.
    root->set_sources();

    // Clean up dumped files and modules on uninstall.
    recurse_uninstall_file(root->sub_dirs["lib"], "*.o");

    // Note: TARGETDIR is the root install dir for the msi,
    // PIKE_TARGETDIR might be a different dir if Pike_module.wxs is
    // included in an installer that wants to install pike somewhere
    // else.

    // We need to have a unique name for the TARGETDIR
    // due to light not rewriting [TARGETDIR] in the
    // property custom action below.
    //
    // Note: The directory we get merged into must also have
    //       this id as long as the bug exists.
    root->extra_ids["PIKE_TARGETDIR"] = 1;

    // Generate the XML directory tree.
    WixNode xml_root =
      Standards.XML.Wix.get_module_xml(root, "Pike", version_str,
				       "IDA", "Pike dist", version_guid,
				       "Merge with this", "300");

    WixNode module_node = xml_root->
      get_first_element("Wix")->
      get_first_element("Module");

    module_node->
      add_child(WixNode("CustomAction", ([
			  "Id":"SetFinalizePike",
			  "Property":"FinalizePike",
			  "Value":"[PIKE_TARGETDIR]",
			  "Execute":"immediate",
			])))->
      add_child(line_feed)->
      add_child(WixNode("CustomAction", ([
			  "Id":"FinalizePike",
			  "BinaryKey":"PikeInstaller",
			  "VBScriptCall":"FinalizePike",
			  // The following are necessary to allow the script to
			  // run with elevated privileges in UAC mode.
			  "Execute":"deferred",
			  "Impersonate": "no",
			])))->
      add_child(line_feed)->
      add_child(WixNode("Binary", ([
			  "Id":"PikeInstaller",
			  "src":"PikeWin32Installer.vbs",
			])))->
      add_child(line_feed)->
      add_child(WixNode("InstallExecuteSequence", ([]), "\n")->
		add_child(WixNode("Custom", ([
				    "Action":"SetFinalizePike",
				    "After":"WriteRegistryValues",
				  ]), "REMOVE=\"\""))->
		add_child(line_feed)->
		add_child(WixNode("Custom", ([
				    "Action":"FinalizePike",
				    "After":"SetFinalizePike",
				  ]), "REMOVE=\"\""))->
		add_child(line_feed))->
      add_child(line_feed);

    create_file("Pike_module.wxs", xml_root->render_xml());

#ifdef GENERATE_WIX_UI
    // Generate the UserInterface

    status("Creating", /*export_base_name*/"Pike"+"_ui.wxs");

    xml_root = Parser.XML.Tree.SimpleRootNode()->
      add_child(Parser.XML.Tree.SimpleHeaderNode((["version":"1.0",
						   "encoding":"utf-8"])))->
      add_child(WixNode("Wix", ([
			  "xmlns":"http://schemas.microsoft.com/wix/2003/01/wi",
			]))->
		add_child(WixNode("Fragment", ([
				    "Id":"PikeUI",
				  ]))->
			  add_child(UI()->gen_xml())->
			  add_child(WixNode("Binary", ([
					      "Id":"Pike_banner",
					      "src":"Pike_banner.bmp"
					    ])))));

    Stdio.write_file(/*export_base_name*/"Pike"+"_ui.wxs",
		     xml_root->render_xml());
      
#endif /* GENERATE_WIX_UI */

#ifdef GENERATE_WIX_ACTIONS
    // Generate the custom actions needed to install the master,
    // and finalize the pike binary.

    status("Creating", export_base_name+"_actions.wxs");

    string run_install =
      "bin\\pike -DNOT_INSTALLED"
      " -mbuild\\master.pike bin\\install.pike"
      " BASEDIR=.";
    WixNode fragment_list =
      WixNode("Fragment", ([
		"Id":"PikeActions",
	      ]))->
      add_child(WixNode("CustomAction", ([
			  "Id":"FinalizePike",
			  "Directory":"TARGETDIR",
			  "ExeCommand":run_install + " --finalize",
			])))->
      add_child(WixNode("CustomAction", ([
			  "Id":"InstallMaster",
			  "Directory":"TARGETDIR",
			  "ExeCommand":run_install + " --install-master",
			])));

    xml_root = Parser.XML.Tree.SimpleRootNode()->
      add_child(Parser.XML.Tree.SimpleHeaderNode((["version":"1.0",
						   "encoding":"utf-8"])))->
      add_child(WixNode("Wix", ([
			  "xmlns":"http://schemas.microsoft.com/wix/2003/01/wi",
			]))->
		add_child(fragment_list));

    Stdio.write_file(export_base_name+"_actions.wxs", xml_root->render_xml());

#endif /* GENERATE_WIX_ACTIONS */

#if 0
    // Generate the main wxs file.

    status("Creating", export_base_name+".wxs");

    WixNode product_node = WixNode("Product", ([
				     "Name":"Pike",
				     "Language":"1033",
				     "UpgradeCode":pike_upgrade_guid,
				     "Id":Standards.UUID.make_version1(-1)->str(),
				     "Version":sprintf("%d.%d.%d",
						       __REAL_MAJOR__,
						       __REAL_MINOR__,
						       __REAL_BUILD__),
				     "Manufacturer":"IDA",
				   ]))->
      add_child(WixNode("Package", ([
			  "Manufacturer":"IDA",
			  "Languages":"1033",
			  "InstallerVersion":"300",
			  "Platforms":"Intel",
			  "Id":Standards.UUID.make_version1(-1)->str(),
			  "Compressed":"yes",
			  "SummaryCodepage":"1252",
			])))->
      add_child(WixNode("Media", ([
			  "Id":"1",
			  "EmbedCab":"yes",
			  "Cabinet":"Pike.cab",
			])))->
      add_child(WixNode("Directory", ([
			  "Id":"TARGETDIR",
			  "Name":"SourceDir",
			]))->
		add_child(WixNode("Merge", ([
				    "Id":"Pike",
				    "Language":"1033",
				    "src":export_base_name+"_module.msm",
				    "DiskId":"1",
				  ]))))->
      add_child(WixNode("Feature", ([
			  "Id":"F_Pike",
			  "Title":sprintf("Pike %d.%d.%d",
					  __REAL_MAJOR__,
					  __REAL_MINOR__,
					  __REAL_BUILD__),
			  "Level":"1",
			  "ConfigurableDirectory":"TARGETDIR",
			]))->
		add_child(WixNode("MergeRef", ([
				    "Id":"Pike",
				  ]))))->
      add_child(WixNode("Upgrade", ([
			  "Id":Standards.UUID.make_version1(-1)->str(),
			]))->
		add_child(WixNode("UpgradeVersion", ([
				    "Minimum":sprintf("%d.0.0",
						      __REAL_MAJOR__),
				    "Property":"NEWERPRODUCTFOUND",
				    "OnlyDetect":"yes",
				    "IncludeMinimum":"yes",
				  ]))))->
      add_child(WixNode("CustomAction", ([
			  "Id":"QueryTarget",
			  "Property":"TARGETDIR",
			  "Value":"[ProgramFilesFolder][ProductName]",
			  "Execute":"firstSequence",
			])))
      ->
      add_child(WixNode("InstallExecuteSequence", ([]))->
		add_child(WixNode("Custom", ([
				    "Action":"QueryTarget",
				    "Before":"InstallFiles",
				  ]),
				  "TARGETDIR=\"\""))
#if 0
		->
		add_child(WixNode("Custom", ([
				    "Action":"FinalizePike",
				    "After":"InstallFiles",
				  ])))->
		add_child(WixNode("Custom", ([
				    "Action":"InstallMaster",
				    "After":"FinalizePike",
				  ])))
#endif /* 0 */
		)
#if 0
      ->
      add_child(WixNode("UI", ([]))->
		add_child(WixNode("Dialog", ([
				    "Id":"TargetDialog",
				    "Title":"[ProductName]",
				    "TrackDiskSpace":"yes",
				    "Width":"373",
				    "Height":"287",
				  ]))->
			  add_child(WixNode("Control", ([
					      "Id":"TargetEdit",
					      "Type":"PathEdit",
					      "Property":"TARGETDIR",
					      "Sunken":"yes",
					      "Width":"258",
					      "Height":"18",
					      "X":"18",
					      "Y":"126",
					    ])))->
			  add_child(WixNode("Control", ([
					      "Id":"NextButton",
					      "Type":"PushButton",
					      "Default":"yes",
					      "Width":"66",
					      "Height":"18",
					      "X":"300",
					      "Y":"261",
					    ]))->
				    add_child(WixNode("Publish", ([
							"Event":"SetTarget",
							"Value":"TARGETDIR",
						      ])))->
				    add_child(WixNode("Publish", ([
							"Event":"EndDialog",
							"Value":"Return"
						      ])))))->
		add_child(WixNode("InstallUISequence", ([]))->
			  add_child(WixNode("Custom", ([
					      "Action":"QueryTarget",
					      "Before":"TargetDialog",
					    ])))->
			  add_child(WixNode("Show", ([
					      "Dialog":"TargetDialog",
					      "Before":"ProgressForm",
					    ])))))
#endif /* 0 */
      ;

    xml_root = Parser.XML.Tree.SimpleRootNode()->
      add_child(Parser.XML.Tree.SimpleHeaderNode((["version":"1.0",
						   "encoding":"utf-8"])))->
      add_child(WixNode("Wix", ([
			  "xmlns":"http://schemas.microsoft.com/wix/2003/01/wi",
			]))->add_child(product_node));

    Stdio.write_file(export_base_name+".wxs", xml_root->render_xml());
#endif /* 0 */
  }

  void low_finalize_pike(string pike_bin_file, string suffix)
  {
    ::low_finalize_pike(pike_bin_file, suffix);
    low_install_regkey("bin", "HKLM",
		       "SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\Parameters\\FirewallPolicy\\StandardProfile\\AuthorizedApplications\\List",
		       "[#BIN_PIKE]",
		       "[#BIN_PIKE]:*:Enabled:Pike",
		       "RE__BIN_PIKE");
    low_uninstall_file("bin/*.old");
  }
}
#endif /* SUPPORT_WIX */

class AmigaOSExportInstallHandler {
  inherit ExportInstallHandler;
  protected array(array(string)) to_export=({});

  protected void generate_prefix()
  {
  }

  protected void create_export_base()
  {
    if (!mkdir(export_base_name)) {
      error("Failed to create directory %O: %s\n",
	    export_base_name, strerror(errno()));
    }
    vars->TMP_BUILDDIR=getcwd();
    cd(export_base_name);
    vars->prefix = prefix = "";
    exec_prefix = "";
  }

  protected string get_export_base_name(string ver)
  {
    return sprintf("%s-AmigaOS", ver);
  }

  void export_file (string from, string tmp, string to, void|string id)
  {
    to_export += ({({from, to})});
  }

  protected void do_install_master(string master_src)
  {
    make_master("master.pike", master_src, "PROGDIR:lib", "PROGDIR:include/pike",
		0, cflags, ldflags);
    low_install_file(combine_path(getcwd(), "master.pike"), "lib/master.pike");
  }
  
  protected void write_num_files_to_export()
  {
  }

  protected void do_install_build_aux()
  {
    low_install_file(combine_path(vars->SRCDIR,
                                  "../packaging/amigaos/InstallPike"),
                     combine_path(prefix, "InstallPike"));
    low_install_file(combine_path(vars->SRCDIR,
                                  "../packaging/amigaos/InstallPike.info"),
                     combine_path(prefix, "InstallPike.info"));
  }

  void do_export()
  {
    allow_mkdirhier = 1;
    set_simple_status(0);

    cd("..");

    [array(string) to_export_from, array(string) to_export_tmp] =
      Array.transpose (to_export);

    // Link in files outside the build tree during the lha.
    for (int i = 0; i < sizeof (to_export_from); i++) {
      string tmpname = to_export_tmp[i];
      to_export_tmp[i] =
	combine_path (export_base_name, to_export_tmp[i]);
      if (to_export_from[i] != tmpname && !file_stat(to_export_tmp[i])) {
	Stdio.mkdirhier(dirname(to_export_tmp[i]));
#if constant(hardlink)
	hardlink (combine_path(getcwd(), to_export_from[i]), to_export_tmp[i]);
#else
	mklink (combine_path(getcwd(), to_export_from[i]), to_export_tmp[i]);
#endif
      }
      else
	to_export_from[i] = 0;
    }

    string tmpmsg=".";

    string lhaarg="cq";
    foreach(to_export_tmp/25.0, array files_to_lha)
    {
      status("Creating", export_base_name+".lha", tmpmsg);
      tmpmsg+=".";
      Process.create_process(({"lha",lhaarg,export_base_name+".lha"})+ files_to_lha)
	->wait();
      lhaarg="aq";
    }

    // Clean up symlinks again.
    for (int i = 0; i < sizeof (to_export_from); i++)
      if (to_export_from[i])
	rm (to_export_tmp[i]);

    status("Cleaning up","");

    Process.create_process( ({ "rm","-rf",
			       export_base_name,
			    }) ) ->wait();

    status1("Export done");
  }
}

int pre_install(array(string) argv)
{
  InstallHandler install_handler;
  string prefix;
  if( vars->prefix )
    prefix = vars->prefix;
  else {
#ifdef __NT__
    prefix = System.RegGetValue(HKEY_LOCAL_MACHINE,
                                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                                "ProgramFilesDir");
#else
    prefix = "/usr/local";
#endif
  }

  if(!vars->TMP_BINDIR)
    vars->TMP_BINDIR=combine_path(vars->SRCDIR,"../bin");

  if(!vars->TMP_BUILDDIR) vars->TMP_BUILDDIR=".";

  cflags = vars->cflags;
  ldflags = vars->ldflags;

  switch(install_type)
  {
    case "--traditional":
      install_handler = TraditionalInstallHandler(vars, prefix);
      break;

    case "--interactive":

#ifdef USE_GTK
      catch  {
	if(!no_gui)
	{
#ifndef __NT__ // We are using GTK on Win32!! no DISPLAY required
	  if(getenv("DISPLAY"))
#endif
	  {
	    begin_wizard(argv, prefix);
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

	prefix = interactive->edit_directory(prefix,"Install prefix: ");
	prefix = interactive->absolute_path(prefix);

	if(!vars->pike_name)
	{
#if constant(symlink)
	  bin_path=interactive->edit_filename
		   (combine_path(vars->exec_prefix ||
				 combine_path(prefix, "bin"),
				 "pike"), "Pike binary name: ");
#else
	  bin_path=combine_path(
#ifdef __NT__
				"\\",
#else
				"/",
#endif
				getcwd(),prefix,"pike",
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
      install_handler = NewStyleInstallHandler(vars, prefix);
      break;

    case "--export-amigaos":
      install_handler = AmigaOSExportInstallHandler(vars, prefix);
      break;
    case "--wix-module":
#ifdef SUPPORT_WIX
      install_handler = WixExportInstallHandler(vars, prefix);
#else
      error("Wix mode not supported.\n");
#endif
      break;
    case "--export":
#ifdef __NT__
      install_handler = BurkExportInstallHandler(vars, prefix);
#else
      install_handler = TarExportInstallHandler(vars, prefix);
#endif
      break;

    case "":
    default:
    case "--new-style":
      install_handler = NewStyleInstallHandler(vars, prefix);
      break;
    case "--finalize":
      install_handler = InstallHandler(vars, getcwd());
      install_handler->finalize_pike();
      status1("Finalizing done.");
      return 0;
    case "--install-master":
      install_handler = InstallHandler(vars, getcwd());
      install_handler->make_master("lib/master.pike", "lib/master.pike.in",
				   lib_prefix, include_prefix, UNDEFINED,
				   cflags, ldflags);
      status1("Installing master done.");
      return 0;

  case "--wix":
#ifdef SUPPORT_WIX
    WixInstallHandler()->make_wix();
#else /* !SUPPORT_WIX */
    error("Wix mode not supported with this pike.\n");
#endif /* SUPPORT_WIX */
    return 0;
  }

  install_handler->do_install();
  return 0;
}

void collect_dlls (function(string:void) handle_dll)
{
  // $PIKE_BUILD_ROOT/dll contains placeholders for the dlls that need
  // to be included in the install package. install.pike searches for
  // these in PATH. The contents of the files here are not important;
  // they can be zero length. The MS CRT dlls shouldn't be here,
  // though (see add_msm instead).
  if (string pike_build_root = getenv ("PIKE_BUILD_ROOT"))
    if (array(string) dlls = get_dir (combine_path (pike_build_root, "dll"))) {
    dll_loop:
      foreach (dlls, string dll_name) {
	dll_name = lower_case (dll_name);
	if (has_suffix (dll_name, ".dll")) {
	  foreach (getenv ("PATH") / ";", string path) {
	    string dll_path = combine_path (path, dll_name);
	    if (Stdio.exist (dll_path)) {
	      handle_dll (normalize_path (dll_path));
	      continue dll_loop;
	    }
	  }
	  error_msg ("Warning: Could not find dll %s to include.\n", dll_name);
	}
      }
    }
}

int main(int argc, array(string) argv)
{
  foreach(Getopt.find_all_options(argv, ({
    ({"help",Getopt.NO_ARG,({"-h","--help"})}),
    ({"notty",Getopt.NO_ARG,({"-t","--notty"})}),
    ({"--interactive",Getopt.NO_ARG,({"-i","--interactive"})}),
    ({"--new-style",Getopt.NO_ARG,({"--new-style"})}),
    ({"--finalize",Getopt.NO_ARG,({"--finalize"})}),
    ({"--install-master",Getopt.NO_ARG,({"--install-master"})}),
    ({"no-autodoc",Getopt.NO_ARG,({"--no-autodoc","--no-refdoc"})}),
    ({"no-gui",Getopt.NO_ARG,({"--no-gui","--no-x"})}),
    ({"--export",Getopt.NO_ARG,({"--export"})}),
    ({"--wix", Getopt.NO_ARG, ({ "--wix" })}),
    ({"--wix-module", Getopt.NO_ARG, ({ "--wix-module" })}),
    ({"--export-amigaos",Getopt.NO_ARG,({"--export-amigaos"})}),
    ({"--traditional",Getopt.NO_ARG,({"--traditional"})}),
    ({"--verbose",Getopt.NO_ARG,({"--verbose"})}),
    ({"--redump-all",Getopt.NO_ARG,({"--redump-all"})}),
    ({"--release-crt",Getopt.NO_ARG,({"--release-crt"})}),
    ({"--debug-crt",Getopt.NO_ARG,({"--debug-crt"})}),
    ({"--collect-dlls", Getopt.HAS_ARG, ({"--collect-dlls"})}),
    }) ),array opt)
    {
      switch(opt[0])
      {
        case "no-autodoc":
	  no_autodoc = 1;
	  break;

	case "no-gui":
	  no_gui=1;
	  break;

	case "help":
	  write(helptext);
	  exit(0);

	case "notty":
	  istty_cache=-1;
	  break;

	case "--verbose":
	  verbose=1;
	  break;

        case "--redump-all":
	  redump_all = 1;
	  break;

	  // The following two are used to install the right dlls for
	  // Microsoft CRT when a dynamic crt is used.
	case "--release-crt":
	  include_crt = "release";
	  break;
	case "--debug-crt":
	  include_crt = "debug";
	  break;

	case "--collect-dlls":
	  // A helper to collect all the dlls, to make it easier to
	  // check that the dll list is complete.
	  collect_dlls (lambda (string dll_path) {
			  werror ("Copying dll %s\n", dll_path);
			  string target =
			    combine_path (opt[1], basename (dll_path));
			  if (!Stdio.cp (dll_path, target))
			    error_msg ("Failed to copy dll to %s: %d\n",
				       target, errno());
			});
	  return 0;

	default:
	  install_type=opt[0];
      }
    }

  argv=Getopt.get_args(argv);

  foreach(argv[1..], string foo)
    if(sscanf(foo,"%s=%s",string var, string value)==2)
      vars[var]=value;

  if(vars->BASEDIR) {
    if(vars->BASEDIR[-1]!='/') vars->BASEDIR += "/";
    if(!vars->LIBDIR_SRC) vars->LIBDIR_SRC=vars->BASEDIR+"lib";
    if(!vars->MANDIR_SRC) vars->MANDIR_SRC=vars->BASEDIR+"share/man";
    if(!vars->DOCDIR_SRC) vars->DOCDIR_SRC=vars->BASEDIR+"refdoc";
    if(!vars->SRCDIR) vars->SRCDIR=vars->BASEDIR+"src";
  }
  else if(vars->SRCDIR) {
    // Do some guessing...
    array split = vars->SRCDIR/"/";
    vars->BASEDIR = split[..sizeof(split)-2]*"/"+"/";
  }
  else {
    werror("No BASEDIR.\n");
    exit(1);
  }

  // Some magic for the fakeroot stuff
  string tmp = m_delete(vars, "fakeroot");
  if(tmp && tmp!="")
    FakerootSetupHandler(vars, tmp);
  return pre_install(argv);
}
