#pike __REAL_VERSION__

//! Create a Position object from a RT38 coordinate
class PositionRT38 {
  inherit .Position;
  void create(int|float|string x, int|float|string y) {
    ::create();
    set_from_RT38(x,y);
  }

  string _sprintf(int|void t)
  {
    return t=='O' && sprintf("%O(%f, %f)", this_program, @RT38());
  }
}
