/* Atom.pmod
 *
 * X Atoms
 *
 */

class Atom
{
  object display;
  string name;
  int id;

  void create(object d /*, int i, string|void n */)
  {
    display = d;
    // id = i;
    // name = n;
  }
}

class pending_request
{
  object display;
  object atom;
  function callback;

  void create(object d, object a, function|void c)
  {
    display = d;
    atom = a;
    callback = c;
  }
}

class pending_intern
{
  inherit pending_request;
  
  void handle_reply(int success, mixed reply)
  {
    if (!success)
      {
	throw( ({ "Atom.pending_intern->handle_reply: InternAtom failed!\n",
		    backtrace() })  );
      }
    atom->id = reply;
    display->remember_atom(atom);
    if (callback)
      callback(atom);
  }
}

class pending_name_lookup
{
  inherit pending_request;
  
  void handle_reply(int success, mixed reply)
  {
    if (!success)
      {
	throw( ({ "Atom.pending_intern->handle_reply: InternAtom failed!\n",
		    backtrace() })  );
      }
    atom->name = reply;
    display->remember_atom(atom);
    if (callback)
      callback(atom);
  }
}

/* Keeps track of known atoms. *
 * Is inherited into Xlib.Display */
class atom_manager
{
  mapping(int:object) atoms = ([ ]);
  mapping(string:object) atom_table = ([ ]);

  /* Defined in Xlib.display */
  void send_async_request(object req, function callback);
  array blocking_request(object req);
  
  void remember_atom(object atom)
  {
    atoms[atom->id] = atom;
    atom_table[atom->name] = atom;
  }

  object InternAtom_req(string name)
  {
    object req = Requests.InternAtom();
    req->name = name;
    return req;
  }

  /* Looks up the atom in local cache. If it is not present,
   * issue an asyncronous InternAtom request, and return 0 */
  object InternAtom(string name, function|void callback)
  {
    if (atom_table[name])
      return atom_table[name];

    object atom = Atom(this_object());
    atom->name = name;

    object req = Requests.InternAtom();
    req->name = name;
    if (callback)
      {
	send_async_request(req, pending_intern(this_object(), atom,
					       callback)->handle_reply);
	return 0;
      } else {
	array a = blocking_request(req);
	if (!a[0])
	  return 0;
	atom->id = a[1];
	remember_atom(atom);
	return atom;
      }
  }

  object GetAtomName_req(object atom)
  {
    object req = Requests.GetAtomName();
    req->atom = atom->id;
    return req;
  }
  
  object lookup_atom(int id, function|void callback)
  {
    if (atoms[id])
      return atoms[id];
    object atom = Atom(this_object());
    atom->id = id;
    object req = GetAtomName_req(atom);
    if (callback)
      {
	send_async_request(req, pending_name_lookup(this_object(), atom,
						  callback)->handle_reply);
	return 0;
      } else {
	array a = blocking_request(req);
	if (!a[0])
	  return 0;
	atom->name = a[1];
	remember_atom(atom);
	return atom;
      }
  }

  void create()
  {
    for(int i = 1; i<sizeof(_Xlib.predefined_atoms); i++)
      {
	object atom = Atom(this_object());
	atom->id = i;
	atom->name = _Xlib.predefined_atoms[i];
	remember_atom(atom);
      }
  }
}
