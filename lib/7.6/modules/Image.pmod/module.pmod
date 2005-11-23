
#pike 7.7

inherit Image;

static class _Image {
  inherit Image;

  array(Color) select_colors(int n) {
    return (array)Colortable(this, n);
  }

  Image map_closest(array(array(int)) t) {
    return Colortable(t)->map(this);
  }

  Image map_fast(array(array(int)) t) {
    return Colortable(t)->map(this);
  }

  Image map_fs(array(array(int)) t) {
    return Colortable(t)->floyd_steinberg()->map(this);
  }
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
  case "colortable":
    index = "Colortable";
    break;
  }
  return ::`[](index);
}
