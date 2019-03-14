
#pike 7.7

inherit Image.Image;

//! Do @expr{(array)Image.Colortable(this, n)@} instead.
//! @deprecated
array(Image.Color) select_colors(int n) {
  return (array)Image.Colortable(this, n);
}

//! Do @expr{Image.Colortable(t)->map(this)@} instead.
//! @deprecated
Image.Image map_closest(array(array(int)) t) {
  return Image.Colortable(t)->map(this);
}

//! Do @expr{Image.Colortable(t)->map(this)@} instead.
//! @deprecated
Image.Image map_fast(array(array(int)) t) {
  return Image.Colortable(t)->map(this);
}

//! Do
//! @expr{Image.Colortable(t)->floyd_steinberg()->map(this)@}
//! instead.
//! @deprecated
Image.Image map_fs(array(array(int)) t) {
  return Image.Colortable(t)->floyd_steinberg()->map(this);
}
