//
// $Id: Events.pmod,v 1.3 2004/04/08 21:52:24 nilsson Exp $
//

#pike __REAL_VERSION__

//! GLUE Event abstraction.

constant _SHFT = 1; //! Integer constant representing shift.
constant _CTRL = 4; //! Integer constant representing control.
constant _ALT  = 8; //! Integer constant representing alternate.

//! Integer constant with the union of all known modifiers, i.e.
//! @expr{_SHFT | _CTRL | _ALT@}.
constant KNOWN_MODIFIERS = _SHFT | _CTRL | _ALT;

// @decl Event STATE(int X, int Y)
// @decl array(Event) STATE(array(int|Event) X, int Y)
// @decl Event STATE(Event X, int Y)
static array(Event)|Event STATE(array|Event|int X,int Y)
{
  if( intp( X ) )    return Event( X,1,0,Y );
  if( arrayp( X ) )   return map( X, STATE, Y );
  X = X->dup();
  X->modifiers |= Y;
  return X;
}

//! @decl Event CTRL(int|Event X)
//! @decl array(Event) CTRL(array(int|Event) X)
//! Adds the @[_CTRL] modifier to an @[Event], key or array of Events
//! and/or keys.
array(Event)|Event CTRL(int|array|Event X) {
  return STATE( X, _CTRL );
}

//! @decl Event SHFT(int|Event X)
//! @decl array(Event) SHFT(array(int|Event) X)
//! Adds the @[_SHFT] modifier to an @[Event], key or array of Events
//! and/or keys.
array(Event)|Event SHFT(int|array|Event X) {
  return STATE( X, _SHFT );
}

//! @decl Event ALT(int|Event X)
//! @decl array(Event) ALT(array(int|Event) X)
//! Adds the @[_ALT] modifier to an @[Event], key or array of Events
//! and/or keys.
array(Event)|Event ALT(int|array|Event X) {
  return STATE( X, _ALT );
}

//! Numeric constant representing a key.
constant BACKSPACE = 65288;
constant DELETE = 65535;
constant TAB = 65289;
constant F1 = 65470;
constant F2 = F1+1;
constant F3 = F2+1;
constant F4 = F3+1;
constant F5 = F4+1;
constant F6 = F5+1;
constant F7 = F6+1;
constant F8 = F7+1;
constant F9 = F8+1;
constant F10 = F9+1;
constant F11 = F10+1;
constant F12 = F11+1;
constant ESCAPE = 65307;
constant UP = 65362;
constant DOWN = 65364;
constant LEFT = 65361;
constant RIGHT = 65363;
constant PGUP  = 65365;
constant PGDWN = 65366;
constant ENTER = 65293;
constant SPACE = 32;
constant HOME = 0xff50;
constant END =  0xff57;
constant PAUSE =0xff13;
constant INSERT=0xff63;
constant SCROLL_LOCK=0xff14;
constant SYS_REQ = 0xff61; // not really, it's print
constant PRINT_SCRN = 0xff61;
constant CAPSLOCK = 0xffe5;
constant MENU = 0xff67;
constant NUMLOCK = 0xff7f;
constant A = 'a';
constant B = 'b';
constant C = 'c';
constant D = 'd';
constant E = 'e';
constant F = 'f';
constant G = 'g';
constant H = 'h';
constant I = 'i';
constant J = 'j';
constant K = 'k';
constant L = 'l';
constant M = 'm';
constant N = 'n';
constant O = 'o';
constant P = 'p';
constant Q = 'q';
constant R = 'r';
constant S = 's';
constant T = 't';
constant U = 'u';
constant V = 'v';
constant W = 'w';
constant X = 'x';
constant Y = 'y';
constant Z = 'z';


//! Numeric constant representing a mouse button.
constant BUTTON_1 = -1;
constant BUTTON_2 = -2;
constant BUTTON_3 = -3;
constant BUTTON_4 = -4;
constant BUTTON_5 = -5;

//! Numeric constant representing a mouse movement.
constant MOUSE_UP = -10;
constant MOUSE_DOWN = -11;
constant MOUSE_LEFT = -12;
constant MOUSE_RIGHT = -13;
constant MOUSE_ABS = -14;

//! Numeric constant representing a modifier key.
constant LSHIFT    = 100000000;
constant RSHIFT    = 100000001;
constant LCTRL     = 100000002;
constant RCTRL     = 100000003;
constant LALT      = 100000004;
constant RALT      = 100000005;

//! Mapping that maps a modifier key to any of the symbolic modifiers
//! @[_SHFT], @[_CTRL] and @[_ALT].
constant MODIFIERS = ([
  LSHIFT : _SHFT,
  RSHIFT : _SHFT,
  LCTRL : _CTRL,
  RCTRL : _CTRL,
  LALT : _ALT,
  RALT : _ALT,
]);

//! Numeric constant representing an exit event.
constant EXIT = -0xfffff;

//! Mapping that maps key identifiers with a printable name, e.g.
//! @[LSHIFT] to @expr{"Left shift"@}.
constant key_names =
([
  LSHIFT: "Left shift",  RSHIFT: "Right shift",
  LCTRL: "Left control",  RCTRL: "Right control",
  LALT: "Left alt",  RALT: "Right alt",

  F1: "F1",  F2: "F2",  F3: "F3",  F4: "F4",  F5: "F5",
  F6: "F6",  F7: "F7",  F8: "F8",  F9: "F9",  F10: "F10",
  F11: "F11",  F12: "F12",

  BACKSPACE: "Backspace", DELETE: "Delete", TAB: "Tab",
  ESCAPE: "Escape", UP: "Up", DOWN: "Down", LEFT: "Left",
  RIGHT: "Right", PGUP: "Page Up", PGDWN: "Page Down",
  ENTER: "Enter", SPACE: "Space", HOME: "Home", END: "End",
  PAUSE: "Pause", INSERT: "Insert",
  SCROLL_LOCK: "Scroll lock", PRINT_SCRN: "Print",
  CAPSLOCK: "Caps lock", NUMLOCK: "Numeric lock", MENU: "Menu",

  BUTTON_1: "Mouse 1", BUTTON_2: "Mouse 2",
  BUTTON_3: "Mouse 3", BUTTON_4: "Wheel up",
  BUTTON_5: "Wheel down",

  MOUSE_UP: "Mouse up", MOUSE_DOWN: "Mouse down",
  MOUSE_LEFT: "Mouse left", MOUSE_RIGHT: "Mouse right",

  -100:"Joy0",  -101:"Joy1",  -102:"Joy2",  -103:"Joy3",
  -104:"Joy4",  -105:"Joy5",  -106:"Joy6",  -107:"Joy7",
  -108:"Joy8",  -109:"Joy9",  -110:"Joy10",  -111:"Joy11", 
  -112:"Joy12", -113:"Joy13",  -114:"Joy14",  -115:"Joy15",
  -116:"Joy16", -117:"Joy17",  -118:"Joy18",  -119:"Joy19",

  -1001:"Joy X+", -1002:"Joy X-",
  -2001:"Joy Y+", -2002:"Joy Y-",
  -3001:"Joy Z+", -3002:"Joy Z-",
  -4001:"Joy 4+", -4002:"Joy 4-",
  -5001:"Joy 5+", -5002:"Joy 5-",
  -6001:"Joy 6+", -6002:"Joy 6-",
  -7001:"Joy 7+", -7002:"Joy 7-",
  -8001:"Joy 8+", -8002:"Joy 8-",
  -9001:"Joy 9+", -9002:"Joy 9-",
  -10001:"Joy 10+", -10002:"Joy 10-",

  EXIT:"Exit",
]);

#ifdef __NT__
string get_nt_keymap() {
  string keymap = RegGetValue(HKEY_CURRENT_USER,
			      "Keyboard Layout\\Preload", "1");
  keymap = RegGetValue(HKEY_LOCAL_MACHINE,
		       "SYSTEM\\CurrentControlSet\\Control\\"
		       "Keyboard Layout\\DosKeybCodes", keymap);
  return keymap;
}
#endif

//! Returns @expr{1@} if the key code @[k] is a modifier key, e.g.
//! @[LSHIFT] or @[RSHIFT].
int(0..1) is_modifier(int k)
{
  if( k<0 )
    return 0;
  k &= (1<<28)-1;
  return k >= 0xffe0 && k<0xffef;
}

//! Contains an event.
class Event
{
  //! Returns a copy of this Event object.
  this_program dup()
  {
    this_program q = this_program();
    foreach( ({ "data", "name", "key", "raw", "real_key", "modifiers",
		"pressure", "press", "is_pseudo", "repeat" }),
	     string x )
      q[x] = this[x];
    return q;
  }

  string data;
  string name;

  int key, raw, real_key;
  int modifiers;

  //! The pressure of the key stroke. A value between 0.0 and 1.0.
  //! Unknown values are represented as 0.
  float pressure;

  //! Press event or release event.
  int(0..1) press;

  int is_pseudo;

  int repeat; // number of keyrepeats caused so far from this event.

  array _encode() {
    return ({ key, modifiers, raw });
  }

  void _decode( array data ) {
    create(data[0], 1, 0, data[1]);
    raw = data[2];
    repeat = 0;
  }

  static int(0..1) `>( Event e )
  {
    if( !objectp( e ) )
      return 0;
    return (e->press > press) && (e->key > key) || (e->modifiers > modifiers);
  }

  static int(0..1) `==( Event|int e )
  {
    if( objectp( e ) )
      return ((e->press == press) && (e->key == key) &&
	      (e->modifiers == modifiers) && (e->raw == raw));
  }

  static int __hash( )
  {
    return hash( sprintf( "%d %d %d", press, key, modifiers ) );
  }

  string cast(string to) {
    if(to=="string") return data;
    error("Can not cast to %s.\n", to);
  }

  void update_modifiers(int _modifiers) {
    modifiers = _modifiers;
    if( modifiers == _SHFT )
      data = upper_case(data);
    else if( !modifiers )
      data = lower_case(data);
  }

  static string _sprintf( int t )
  {
    if( t!='O' ) return 0;
    if( !key )
      return "NullEvt()";
    string pressure = (pressure?"?":sprintf("%.1f", pressure));
    return sprintf("%s(%s<%d> %x %O (%s) @ %s )",
		   (press?"Press":"Release"),
		   (modifiers&_SHFT?"shift ":"")+(modifiers&_CTRL?"ctrl ":"")+
		   (modifiers&_ALT?"alt ":"")+ (raw?"raw ":""),repeat,
		   key, data, name||"unnamed", pressure );
  }

  //!
  static void create( int|void _key, int(0..1)|void _press,
		      string|void _data, int|void _modifiers,
		      float|void pressure)
  {
    name = key_names[ _key ];
    key = _key;
    press = _press;
    data = _data || (key > 31 && key < 256 ? sprintf("%c", key ) : "" );
    update_modifiers( _modifiers );
    if( !name )
      name = (key > 31 && key < 256 ? sprintf("%c", key ) : data);
  }
}

int mod_state;

Event key_event(int key, int(0..1) press, void|string data) {
  int state = MODIFIERS[key];
  if(state) {
    if(press)
      mod_state |= state;
    else
      mod_state &= ~state;
    Event e = Event(key, press, data);
    e->raw = 1;
    return e;
  }

  Event e = Event(key, press, data);
  e->modifiers = mod_state;
  return e;
}
