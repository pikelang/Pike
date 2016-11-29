
protected Tools.Sass.SCSS compiler;

constant BASE = __DIR__;

int main(int argc, array(string) argv)
{
  if (argc > 1 && argv[1] == "v") {
    write("libsass version:   %s\n", Tools.Sass.libsass_version());
    write("sass2scss version: %s\n", Tools.Sass.sass2scss_version());
    return 0;
  }

  compiler = Tools.Sass.SCSS();
  compiler->set_options(([
    "output_style" : Tools.Sass.STYLE_EXPANDED
  ]));

  run();

  return 0;
}

int run()
{
  error("Method run() not implemented!\n");
}
