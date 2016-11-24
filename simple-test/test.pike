
protected Tools.Sass.SCSS compiler;

constant BASE = __DIR__;

int main(int argc, array(string) argv)
{
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
