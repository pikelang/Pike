# pike-libsass
A Pike module wrapper for libsass.

How to install
_______________

First off you'll need [`libsass`](http://sass-lang.com/libsass) installed.
In Ubuntu/Debian you can get it via `sudo apt install libsass`.

* `git clone https://github.com/poppa/pike-libsass.git`

* `cd pike-libsass`

* You might have to run `autoheader`

* `pike -x module` to compile the module. If all goes well...

* `pike -x module install`


How to use
__________

It's pretty straight forward to use:

```pike
import Tools.Sass;

Compiler compiler = Compiler();

// Allow imports over HTTP. HTTP_IMPORT_GREEDY will raise an error if the
// content type of the imported file isn't text/scss or text/sass.
compiler->http_import = HTTP_IMPORT_GREEDY;

compiler->set_options(([
  "output_style"    : STYLE_COMPRESSED,  // Minifies the output
  "source_map_file" : "my.source.map"    // Will write a source.map file
]));

// Compile my.scss to my.css. my.source.map will also be written.
if (mixed e = catch(compiler->compile_file("my.scss", "my.css"))) {
  werror("Compilation failed: %s\n", describe_error(e));
}
```

And that's that for now.
