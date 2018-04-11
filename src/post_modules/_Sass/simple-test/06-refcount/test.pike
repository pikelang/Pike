inherit "../test.pike";

constant BASE = __DIR__;

int run()
{
  werror("Run in test: %s\n", basename(BASE));
  Tools.Sass.Compiler c;

  mixed err = catch {
    {
      c = Tools.Sass.Compiler();
      c->compile_file(combine_path(BASE, "input.scss"));
    }

    if (array x = Pike.identify_cycle(c)) {
      error("Compiler not freed after scope: %O\n", x);
    }
  };

  handle_err(err);
}
