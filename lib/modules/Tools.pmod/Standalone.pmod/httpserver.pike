#pike __REAL_VERSION__
//#pragma strict_types

constant version = sprintf(#"Pike httpserver %d.%d.%d
",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);

constant description = "Minimal HTTP-server.";

class Options
{
  inherit Arg.Options;

  constant help_pre = #"Usage: httpserver [flags] [path]
Starts a simple HTTP server on port 8080 unless another port is specified. The
server will present the contents of the current directory and it's children to
the world without any authentication.
";

  constant port_help = "Port to use. Defaults to 8080.";
  constant version_help = "Displays version information.";
  constant headers_help = "Set additional header and value, e.g. --header X-Content-Type-Options:nosniff";
  constant allow_help = "Limit access to a specific IP, IP-range or CIDR net.";
  constant log_help = "Logs request in 'commonlog', 'raw' or 'string' format.";

  Opt port = Int(HasOpt("--port")|Default(8080));
  Opt version = NoOpt("--version");
  Opt headers = Multiple(HasOpt("--header"));
  Opt allow = Multiple(HasOpt("--allow"));
  Opt log = HasOpt("--log");
}

Options opt;
mapping headers = ([]);
NetUtils.IpRangeLookup ip_whitelist;

string cwd;

int main(int argc, array(string) argv)
{
  opt = Options(argv);

  if(opt->version)
    exit(0, version);
  if(opt->help)
    exit(0);

  int port = opt->port;
  if(sizeof(argv=opt[Arg.REST]))
  {
    string home = combine_path(getcwd(), argv[-1]);
    if( Stdio.is_dir(home) )
      cd(home);
    else if(port==8080 && (int)argv[-1])
      port=(int)argv[-1];
  }

  if( opt->headers )
  {
    foreach(opt->headers, string hdr)
    {
      array h = hdr/":";
      if(sizeof(h)<2) error("Illegal header format %O.\n", hdr);
      headers[h[0]] = h[1..]*":";
    }
  }

  if( opt->allow )
  {
    ip_whitelist = NetUtils.IpRangeLookup( ([ 1 : opt->allow ]) );
  }

  cwd = getcwd();
  Protocols.HTTP.Server.Port(handle_request, port, 0);
  write("%s is now accessible on port %d through http, "
        "without password.\n", getcwd(), port);
  return -1;
}

string dirlist( string dir, string rel_dir )
{
    string res =
	"<html><head>\n"
        "<style>a { text-decoration: none;  }\n"
        ".odd { background-color:#efefef; }\n"
	".even { background-color:#fefefe; }\n"
        "</style>\n"
	"</head><body>\n"
	"<h1>"+Parser.encode_html_entities(rel_dir)+"</h1>"
        "<table cellspacing='0' cellpadding='2'>\n"
        "<tr><th align='left'>Filename</th>"
	"<th align='right'>Type</th>"
	"<th align='right'>Size</th></tr>\n";

    foreach( sort( get_dir( dir ) ); int i; string fn )
    {
        string path = "/"+Protocols.HTTP.uri_encode((combine_path(rel_dir, fn)/"/")[*])*"/";
        Stdio.Stat s = file_stat( combine_path(dir, fn) );
        if( !s )
            continue;

        string t = s->isdir?"":Protocols.HTTP.Server.filename_to_type(fn);
        if( t == "application/octet-stream" )
            t = "<span style='color:darkgrey'>unknown</span>";

        string html_link = (!s->isdir && has_suffix(lower_case(fn), ".md"))
            ? sprintf(" <a href='%s?format'>[HTML]</a>", path)
            : "";

        res +=
            sprintf("<tr class='%s'><td><a href='%s%s'>%s%[2]s</a>%[6]s</td>"
		    "<td align='right'>%s</td>"
		    "<td align='right'>%s</td></tr>\n",
                    (i&1?"odd":"even"),
                    path, s->isdir?"/":"",
		    Parser.encode_html_entities(fn), t,
                    s->isdir?"":String.int2size(s->size),
                    html_link);
    }
    return res+"</table></body></html>\n";
}

#if constant(Parser.Markdown)

constant md_style = #"
  body { margin:0 auto; padding:1em 2em; max-width:50em; line-height:1.6;
         font-family:system-ui, sans-serif; color:#24292f; background:#fff; }
  a { color:#0969da; }
  h1, h2 { border-bottom:1px solid #d8dee4; padding-bottom:.2em; }
  h1, h2, h3, h4 { line-height:1.25; margin:1.5em 0 .6em; }
  code { font-family:ui-monospace, monospace; font-size:.9em;
         background:#eff1f3; border-radius:4px; padding:.15em .35em; }
  pre { background:#f6f8fa; border-radius:6px; padding:1em; overflow-x:auto; }
  pre code { background:none; padding:0; }
  blockquote { margin:1em 0; padding:0 1em; color:#57606a;
               border-left:.25em solid #d8dee4; }
  table { border-collapse:collapse; }
  th, td { border:1px solid #d8dee4; padding:.4em .8em; }
  tr:nth-child(even) { background-color:#f6f8fa; }
  img { max-width:100%; }
  hr { border:0; border-top:1px solid #d8dee4; }
  @media (prefers-color-scheme: dark) {
    body { color:#e6edf3; background:#0d1117; }
    a { color:#4493f8; }
    h1, h2, hr, th, td, blockquote { border-color:#30363d; }
    code { background:#21262d; }
    pre, tr:nth-child(even) { background:#161b22; }
    blockquote { color:#9198a1; }
  }
";

//! Render a markdown file as a standalone HTML document. Returns 0 if
//! the file could not be read.
string markdown( string rfile )
{
  string md = Stdio.read_file(rfile);
  if( !md )
    return "<h3>File not found</h3>";

  // Use the first level one heading as document title, if any.
  string title;
  foreach( md/"\n", string line )
    if( sscanf(String.trim(line), "# %s", title)==1 )
      break;
  if( !title || !sizeof(title) )
    title = (rfile/"/")[-1];

  string body;
  if( mixed err = catch( body = Parser.Markdown.parse(md) ) )
  {
    werror("Markdown error in %s: %s", rfile, describe_error(err));
    body = "<pre>" + Parser.encode_html_entities(md) + "</pre>";
  }

  return
    "<!DOCTYPE html>\n"
    "<html><head>\n"
    "<meta charset='utf-8'/>\n"
    "<meta name='viewport' content='width=device-width, initial-scale=1'/>\n"
    "<title>" + replace(title, ({"&","<",">"}),
                        ({"&amp;","&lt;","&gt;"})) + "</title>\n"
    "<style>" + md_style + "</style>\n"
    "</head><body>\n" + body + "</body></html>\n";
}

#endif

string file_not_found(string fname)
{
  return
    "<html><body><h1>File not found</h1>\n"
    "<tt>" + Parser.encode_html_entities(fname) + "</tt><br />\n"
    "</body></html>\n";
}

void handle_request(Protocols.HTTP.Server.Request request)
{
    string file = Protocols.HTTP.uri_decode(request->not_query);
    string rel_file = combine_path_unix("/", file)[1..];
    file = combine_path(cwd, rel_file);
    Stdio.Stat s = file_stat( file );
    int ipblock = ip_whitelist &&
      !ip_whitelist->lookup_range(request->my_fd->query_address());

    switch(opt->log)
    {
    case 1:
    case "commonlog":
      {
        object now = Calendar.now();
        int code = 404;
        if( s ) code = 200;
        if( ipblock ) code = 401;
        write("%s - - [%d/%s/%d:%02d:%02d:%02d %s] %O %d %d\n",
              (request->my_fd->query_address()/" ")[0],
              now->month_day(), now->month_name()[..2], now->year_no(),
              now->hour_no(), now->minute_no(), now->second_no(),
              now->tzname_utc_offset(),
              request->request_raw, code,
              s && s->isreg && s->size); // Not showing generated data.
      }
      break;
    case "raw":
      write("%s\n\n", request->raw);
      break;
    case "string":
      write("%O\n\n", request->raw);
      break;
    default:
      break;
    }

    if( ipblock )
      request->response_and_finish( (["data": "Permission denied for "+
                                      (request->my_fd->query_address()/" ")[0],
                                      "type":"text/plain",
                                      "extra_heads" : headers,
                                      "error":401]) );
    else if( !s )
	request->response_and_finish( (["data":
					file_not_found(request->not_query),
					"type":"text/html",
                                        "extra_heads" : headers,
                                        "error":404]) );
    else if( s->isdir )
        request->response_and_finish( ([ "data":dirlist(file, rel_file),
                                         "extra_heads" : headers,
                                         "type":"text/html" ]) );
#if constant(Parser.Markdown)
    else if( has_index(request->variables, "format") )
        request->response_and_finish(
          ([ "data":markdown(file),
             "extra_heads" : headers,
             "type":"text/html; charset=utf-8" ]) );
#endif
    else
      request->response_and_finish( ([ "file":Stdio.File(file),
                                       "extra_heads" : headers,
                                    ]) );
}
