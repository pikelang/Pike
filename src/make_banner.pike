/*
 * $Id: make_banner.pike,v 1.2 2004/12/15 20:58:41 grubba Exp $
 *
 * Make a 500×70 banner BMP image suitable for the Wix installer.
 *
 * 2004-11-02 Henrik Grubbström
 */

int main(int argc, array(string) argv)
{
  // FIXME: Argument parsing bg-color, size, etc.

  if (argc != 2) {
    werror("Usage:\n"
	   "\t%s <source image>\n",
	   argv[0]);
    exit(1);
  }

  string logo_bytes = Stdio.read_bytes(argv[1]);
  if (!logo_bytes) {
    werror("File %O not found.\n", argv[1]);
    exit(1);
  }
  mapping(string:Image.Image) logo = Image._decode(logo_bytes);
  Image.Image banner = Image.Image(500, 70, 255,255,255);
  int margin = (70 - logo->img->ysize())/2;
  banner->paste_mask(logo->img, logo->alpha,
		     500 - (margin + logo->img->xsize()), margin);
  write(Image.BMP.encode(banner));
  return 0;
}