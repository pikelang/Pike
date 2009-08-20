#pike __REAL_VERSION__
//#pragma strict_types

constant doc = #"Usage: httpserver [flags] [port]
Starts a simple HTTP server on port 8080 unless another port is specified. The
server will present the contents of the current directory and it's children to
the world without and authentication.

      --version             print version information and exit
      --help                display this help and exit
";

constant version = sprintf(#"Pike httpserver %d.%d.%d
",(int)__REAL_VERSION__,__REAL_MINOR__,__REAL_BUILD__);

int main(int argc, array(string) argv)
{
    int my_port = 8080;
    if(argc>1)
        switch (argv[-1])
        {
         case "--version":
             exit(0, version);
         case "--help":
             exit(0, doc);
         default:
             my_port=(int)argv[-1];
        }
    Protocols.HTTP.Server.Port(handle_request, my_port);
    write("Listening on " + my_port + "\n");
    return -1;
}

string dirlist( string dir )
{
    string res = 
        "<style>a { text-decoration: none;  }\n"
        ".odd { background-color:#efefef; }\n.even { background-color:#fefefe; }"
        "</style>\n<h1>"+dir[2..]+"</h1>"
        "<table cellspacing='0' cellpadding='2'>"
        "<tr><th align=left>Filename</th><th align=left>Type</th><th align=right>Size</th>";

    foreach( get_dir( dir ); int i; string fn )
    {
        Stdio.Stat s = file_stat( combine_path(dir, fn) );
        if( !s ) 
            continue;

        string t = s->isdir?"":Protocols.HTTP.Server.filename_to_type(fn);
        if( t == "application/octet-stream" )
            t = "<span style='color:darkgrey'>unknown</span>";

        res += 
            sprintf("<tr class='%s'><td><a href='%s%s'>%s%[2]s</a></td><td align=right>%s</td><td align=right>%s</td></tr>\n", 
                    (i&1?"odd":"even"),
                    replace(fn,(["'":"&quot","&":"&amp;"])), 
                    s->isdir?"/":"",
                    replace(fn,(["<":"&lt;","&":"&amp;"])), 
                    t,
                    s->isdir?"":String.int2size(s->size));
    }
    return res+"</table>";
}

void handle_request(Protocols.HTTP.Server.Request request)
{
    string file = "."+combine_path("/",request->not_query);
    Stdio.Stat s = file_stat( file );
    if( !s )
        request->response_and_finish( (["data":"nofile: "+file,"error":404]) );
    else if( s->isdir )
        request->response_and_finish( ([ "data":dirlist(file),"type":"text/html" ]) );
    else
        request->response_and_finish( ([ "file":Stdio.File(file) ]) );
}
