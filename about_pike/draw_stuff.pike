#!/usr/local/bin/pike-mirar 

//Information about Pike is found at
// http://pike.indonex.se 

//This program is made by hedda@idenex.se to demonstrate the Image-package
//in Pike.

//Some understanding of Pike or C is assumed to understand this program.

import Image;

int main()
{
  object(Stdio.File) filefoo;
  filefoo=Stdio.File();                     //Init the file-object
  int k=filefoo->open("foo.ppm", "r");      //Open file foo.ppm for reading


  //Check if foo.ppm exists
  if (k==0)
    {
      write("\nThe picture foo.ppm isn't here! Why?\n");
      return 0;
    }

  object(image) imagefoo; 
  imagefoo=Image.PNM.decode(filefoo->read()); //Read foo.ppm and place in
                                              //the imagefoo-object 

  filefoo->close(); // Close the file foo.ppm

  
  object(image) imagebar=image(52, 65); //Make another image: imagebar.
                                        //This image has width=80 & high=40
                                        //The image will be black

  imagebar->setcolor(160, 240, 192); //The Pike-color! 
  //The color is coded (red, green, blue)
  // 0 means nothing of that color and 255 is maximum 

  //This draws a polygone. It's of cource anti-aliased!
  imagebar->polygone(
		     ({
		       4.1, 50, //First x,y corner
		       7,33, //second x,y corner
		       51, 2, //..
		       45, 28, //The corners can be floats
		       53, 50,
		       9,64 
		     }));  

  //Now we paste the imagebar image onto the imagefoo image
  // "imagebar->threshold(1,1,1)*128" creates the alpha-mask image and
  // is created to be grey where the image imagebar isn't black.
  imagefoo->paste_mask(imagebar, imagebar->threshold(1,1,1)*128, 35, 16);

  object(Stdio.File) filebar;
  filebar=Stdio.File();
  k=filebar->open("bar.gif", "wcx"); //Open bar.gif for writing. Fail if
                                     //it exists.

  if (k==0)
    {
      write("\nSome error while trying to write to the file bar.gif!\n"
	    "Maybe the file already exists.\n");
      return 0;
    }
  
  filebar->write(imagefoo->togif()); // Save the image as a gif to the file
                                     // bar.gif

  filebar->close();  //Close the file bar.gif

  write("\nI have greated the image foo.gif with help of bar.ppm.\n"
	"This achieved by drawing a polygon and pasting"
	" it over the image.\n");
  return 1;
}
