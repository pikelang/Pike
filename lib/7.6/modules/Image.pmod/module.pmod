
#pike 7.7

//! Compat with the Pike 7.6 Image module, which
//! in turn had compat with the Pike 0.6 Image module.
//!
//! Many Image-module classes that were lower-case in 0.6
//! were capitalized during early 0.7.
//!
//! @seealso
//!   @[predef::Image]

//! @decl inherit predef::Image
//! Based on the current Image module.

inherit Image;

//! @ignore
protected class _Image {
//! @endignore

//! @class Image
//!   Compatibility with the old Image API.

  inherit Image;

  array(Color) select_colors(int n) {
    return (array)Colortable(this, n);
  }

  _Image map_closest(array(array(int)) t) {
    return _Image(Colortable(t)->map(this));
  }

  _Image map_fast(array(array(int)) t) {
    return _Image(Colortable(t)->map(this));
  }

  _Image map_fs(array(array(int)) t) {
    return _Image(Colortable(t)->floyd_steinberg()->map(this));
  }

  //! Old versions supported the value @expr{0@} as
  //! shorthand for @[Image.color.black].
  _Image turbulence(array(mixed) colorrange, mixed ... extras) {
    colorrange += ({});
    int i;
    for (i = 1; i < sizeof(colorrange); i += 2) {
      if (!colorrange[i]) colorrange[i] = Color.black;
    }
    ::turbulence(colorrange, @extras);
    return this_object();
  }

//! @ignore
}
//! @endignore

//! @endclass

_Image _load(string fname)
{
  return _Image(::`[]("load")(fname));
}

mixed `[](string index) {
  switch(index) {
  case "font":
    index = "Font";
    break;
  case "Image":
  case "image":
    return _Image;
    break;
  case "color":
    index = "Color";
    break;
  case "colortable":
    index = "Colortable";
    break;
  case "load":
    return _load;
  }
  return ::`[](index);
}
