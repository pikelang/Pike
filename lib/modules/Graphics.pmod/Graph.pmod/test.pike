
//This is a testprogram that creates a simple graph.
#pike __REAL_VERSION__

import ".";

void main()
{
  Image.Image image=
    Graphics.Graph.bars((["data":({({1.0}),({2.0}),({1.0}),({2.0})})]));
  Stdio.write_file("graph.gif", Image.GIF.encode(image));
}
