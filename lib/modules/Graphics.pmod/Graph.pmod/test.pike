
//This is a testprogram that creates a simple graph.
#pike __REAL_VERSION__

import ".";

constant data = ([ "data":
		   ({ ({1.0}), ({2.0}), ({1.0}), ({2.0}) }),
                ]);
mapping verify;

int main(int num, array(string) args)
{
  int w = has_value(args, "-w")||has_value(args, "--write");
  verify = data + ([]);

  Image.Image image;

  image = Graphics.Graph.bars(data);
  if(w) Stdio.write_file("bars.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  image = Graphics.Graph.pie(data);
  if(w) Stdio.write_file("pie.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  image = Graphics.Graph.sumbars(data);
  if(w) Stdio.write_file("sumbars.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  image = Graphics.Graph.line(data);
  if(w) Stdio.write_file("line.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  image = Graphics.Graph.norm(data);
  if(w) Stdio.write_file("norm.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  image = Graphics.Graph.graph(data);
  if(w) Stdio.write_file("graph.gif", Image.GIF.encode(image));
  if(!equal(data,verify))
    error("Destructive change of argument data.\n");

  return 0;
}
