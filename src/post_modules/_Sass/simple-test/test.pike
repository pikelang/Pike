
protected Web.Sass.Compiler compiler;

constant BASE = __DIR__;

int main(int argc, array(string) argv)
{
  if (argc > 1 && argv[1] == "v") {
    write("libsass version:   %s\n", Web.Sass.libsass_version());
    write("sass2scss version: %s\n", Web.Sass.sass2scss_version());
    return 0;
  }

  compiler = Web.Sass.Compiler();
  compiler->set_options(([
    "output_style" : Web.Sass.STYLE_EXPANDED
  ]));

  int rv = run();

  compiler && destruct(compiler);

  return rv;
}

protected int run()
{
  error("Method run() not implemented!\n");
}

protected void handle_err(mixed e)
{
  if (e) {
    werror("%s\n", describe_backtrace(e));
  }
}
