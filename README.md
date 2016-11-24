# pike-libsass
A Pike module wrapper for libsass.

## How to install:

First off you'll need [`libsass`](http://sass-lang.com/libsass) installed.
In Ubuntu/Debian you can get it via `sudo apt install libsass`.

* `git clone https://github.com/poppa/pike-libsass.git`

* `cd pike-libsass`

* `pike -x module` to compile the module. If all goes well...

* `pike -x module install`

## How to use

It's pretty straight forward to use:

```pike
import Tools.Sass;

SCSS compiler = SCSS();

compiler->set_options(([
  "output_style"    : STYLE_COMPRESSED,    // Minifies the output
  "source_map_file" : "my.source.map" // Will write a source.map file
]));

// Compile my.scss to my.css. my.source.map will also be written.
if (mixed e = catch(compile_file("my.scss", "my.css"))) {
  werror("Compilation failed: %s\n", describe_error(e));
}
```

And that's that for now.
