#!/usr/local/bin/pike-hubbe

//This should not be a part of this module?
import ".";

void main()
{
  Image.Image image=Diagram.bars((["data":({({1.0,2.0,1.0,2.0})})]));
  Stdio.write_file("dia.gif", Image.GIF.encode(image));
}
