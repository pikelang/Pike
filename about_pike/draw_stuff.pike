#!/usr/local/bin/pike

// Information about Pike is found at: http://pike.indonex.se 

// This program is made by hedda@idenex.se to demonstrate the Image-package
// in Pike.

// Program cleaned up and made readable by Fredrik Hubinette

// Some understanding of Pike or C is assumed to understand this program.

int main()
{
  // Create an empy file object for file operations
  object(Stdio.File) file=Stdio.File();

  //Check if foo.ppm exists
  if(!file->open("foo.ppm", "r"))
  {
    // If there was an error while opening the file:
    write("\nThe picture foo.ppm isn't here! Why?\n");
    return 0;
  }

  //Read foo.ppm and place in an image-object 
  object(Image.image) image=Image.PNM.decode(file->read());

  file->close(); // Close the file foo.ppm
  
  // Make another image called image2, this image has width=80 & high=40
  // The image will be black
  object(Image.image) image2=Image.image(52, 65);

  // Set the current color to the Pike-green :)
  //The color is coded (red, green, blue)
  // 0 means nothing of that color and 255 is maximum 
  image2->setcolor(160, 240, 192);

  // This draws a polygone. It's of cource anti-aliased!
  image2->polygone(
    ({
      4.1, 50,   // First x,y corner
        7, 33,   // second x,y corner
       51,  2,   // third...
       45, 28,  
       53, 50,   // The corners can also be floats..
        9, 64 
    })
  );  

  // Now we paste the image2 image onto the first image
  // "image2->threshold(1,1,1)*128" creates the alpha-mask image and
  // is created to be grey where the image image2 isn't black.
  image->paste_mask(image2, image2->threshold(1,1,1)*128, 35, 16);

  // Open bar.gif for writing.
  if(!file->open("bar.gif", "wcx"))
  {
    // Fail if the file already exists.
    write("\nSome error while trying to write to the file bar.gif!\n"
	  "Maybe the file already exists.\n");
    return 0;
  }
  
  // Save the image as a gif to the file (bar.gif)
  file->write(image->togif());
  file->close();  // Close the file bar.gif

  write("\nI have greated the image foo.gif with help of bar.ppm.\n"
	"This achieved by drawing a polygon and pasting"
	" it over the image.\n");
  return 1;
}
