#!/usr/bin/env pike

// Converts CHANGES to an XML form used to port change notes on pike.ida.liu.se
// Usage: tools/changes2html.pike CHANGES | xclip   (And then paste into CMS)

constant sub = "---"; //If a line begins with this the line above is a header.

void main(int argc, array argv)
{
    string changes = Stdio.read_file(argv[1]);
    string state = "INIT";
    string out, tmp;
    
    out = "<column charwidth='60'>\n\n";
    
    foreach(changes/"\n", string line)
    {
	line -= "\r";
	//	werror("state: %O\nout: %O\ntmp: %O\nline: %O\n-------\n", state, out, tmp, line);
	switch(state) {
	case "INIT":
	    if(!text(line))
		continue;
	    if(line[0..sizeof(sub)-1] == sub)
	    {
		out += make_header(1, tmp);
		state = "NONE";
		continue;
	    }
	    if(sizeof(line)>=sizeof(sub))
		tmp = line;
	    continue;
	case "NONE":
	    if(!text(line))
		continue;
	    if(line[0..1] == "o ")
	    {
		tmp = line[2..]+"\n";
		out += make_list_start();
		state = "BULLET";
		continue;
	    }
	    tmp = line;
	    state = "HEADER"; //FIXME: large assumption, really should be TEXT.
	    continue;
	case "TEXT":
	    exit(1, "FATAL: unhandled state 'TEXT'\n");
	case "HEADER":
	    //We only want the latest batch of changes.
	    if(has_prefix(tmp, "Changes since Pike"))
	    {
		out += "</column>\n";

		//Tweaks

		//Remove 2nd empty <p>. Should really be done in make_bullet()
		out = replace(out,
			      ({ "\n<p>\n</p><p>\n</p>" }),
			      ({ "\n<p></p>" }));

		//Remove ":" at end of header line
		out = replace(out,
			      ({ ":</b>" }),
			      ({ "</b>" }));

		//Things that still has to be fixed manually:
		// o Sub-bullets (could be automated easily)
		// o One sentence bullets spread over several lines (trivial a)
		// o Run-on descriptions with no clear bullet header (resonable method for some: Let "(Fixed bug .*) ((that|where) .*)" be header: /1, body: /2)
		// o Putting emphasis on "You are encouraged to ugrade just for this fix .*" sentences. (trivial a)

		write(out);
		exit(0);
	    }
	    if(line[0..sizeof(sub)-1] == sub)
	    {
		out += make_header(2, tmp);
		state = "NONE";
		continue;
	    } else
		exit(1, "FATAL: Header state without underline.\n");
	    continue;
	case "BULLET": //broken by header or new bullet (or unindented text?)
	    if(line[0..1] == "o ")
	    {
		out += make_bullet(tmp);
		tmp = line[2..]+"\n";
		state = "BULLET";
		continue;
	    }
	    if(!indented(line))
	    {
		out += make_bullet(tmp);
		out += make_list_end();
		tmp = line;
		state = "HEADER";
		continue;
	    }
	    //not interrupted? add to tmp.
	    tmp += line;
	}
    }
}

string make_bullet(string lines)
{
    string outtmp = "";

    foreach(lines/"\n"; int nr; string line)
    {
	if(nr == 0)
	    outtmp += "<li><b>"+line+"</b>\n<p>\n";
	else
	{
	    if(!text(line))
		outtmp += "</p><p>\n";
	    else
		outtmp += line+"\n";
	}
    }
    outtmp += "</p></li>\n\n";

    return outtmp;
}

string make_list_start()
{
    return "<p><ul>\n\n";
}

string make_list_end()
{
    return "</ul></p>\n\n";
}

int indented(string line)
{
    if(!text(line) || line[0..0] == " ")
	return 1;
    else
	return 0;
}

int text(string line)
{
    if(sizeof(line-" "))
	return 1;
    else
	return 0;
}

string make_header(int level, string text)
{
    return sprintf("<h%d>%s</h%d>\n\n", level, text, level);
}