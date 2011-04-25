// $Id$

#pike __REAL_VERSION__

//! Functions that helps generating HTML. All functions generates
//! HTML that is XHTML compliant as well as backwards compatible
//! with old HTML standards in what extent is possible.

//! Creates an HTML select list.
//!
//! @param name
//!   The name of the select list. Will be used in the name attribute
//!   of the select element.
//! @param choices
//!   May either be an array of strings, where each string is a choice,
//!   or an array of pairs. A pair is an array with two strings. The
//!   first string is the value of the choice while the second string
//!   is the presentation text associated with the value.
//! @param selected
//!   The value that should be selected by default, if any.
//!
//! @example
//!    select("language",
//!      ({ ({ "eng", "English" }),
//!         ({ "swe", "Swedish" }),
//!         ({ "nor", "Norwegian" }) }),
//!      "swe");
string select(string name, array(string)|array(array(string)) choices,
		  void|string selected) {
  string ret = "<select name=\"" + name + "\">\n";

  if(sizeof(choices) && arrayp(choices[0])) {
    foreach([array(array(string))]choices, array(string) value)
      ret += "<option value=\"" + value[0] + "\"" +
	(value[0]==selected?" selected=\"selected\"":"") +
	">" + value[1] + "</option>\n";
  } else {
    foreach([array(string)]choices, string value)
      ret += "<option value=\"" + value + "\"" +
	(value==selected?" selected=\"selected\"":"") +
	">" + value + "</option>\n";
  }

  return ret + "</select>";
}

//! This function should solve most of the obox needs that arises. It
//! creates a table out of the array of arrays of strings fed into it.
//! The tables will (with default settings) have a thin black outline
//! around the table and between its cells. Much effort has gone into
//! finding a simple HTML reresentation of such obox that is rendered
//! in a similar way in all popular browsers. The current
//! implementation has been tested against IE, Netscape, Mozilla,
//! Opera and Konquest.
//!
//! @param rows
//!   Simply an array of arrays with strings. The strings are the
//!   values that should appear in the table cells. All rows should
//!   have equal number of cells, otherwise the result will not be
//!   very eye pleasing.
//! @param frame_color
//!   The color of the surrounding frame. Defaults to "#000000".
//! @param cell_color
//!   The background color of the cells. Defaults to "#ffffff".
//! @param width
//!   The border width. Defaults to "1".
//! @param padding
//!   The amount of padding in each cell. Defaults to "3".
//! @param cell_callback
//!   If provided, the cell callback will be called for each cell. As
//!   in parameters it will get the current x and y coordinates in the
//!   table. The upper left cell is 0,0. In addition to the
//!   coordinates it will also receive the background color and the
//!   contents of the current cell. It is expected to return a
//!   td-element.
//!
//! @example
//!   function cb = lambda(int x, int y, string bgcolor, string contents) {
//!     if(y%2) return "<td bgcolor='#aaaaff'>"+contents+"</td>";
//!     return "<td bgcolor='"+bgcolor+"'>"+contents+"</td>";
//!   }
//!   simple_obox(my_rows, "#0000a0", 0, "1", "3", cb);
//!
//! @seealso
//!   @[pad_rows]
string simple_obox( array(array(string)) rows,
		    void|string frame_color, void|string cell_color,
		    void|string width, void|string padding,
		    void|function(int,int,string,string:string) cell_callback )
{
  .Buffer res = .Buffer();
  if(!cell_color) cell_color = "#ffffff";
  if(cell_callback) {
    foreach(rows; int y; array(string) row) {
      res->add("<tr>");
      foreach(row; int x; string cell)
	res->add( cell_callback(x, y, cell_color, cell) );
      res->add("</tr>");
    }
  }
  else
    foreach(rows, array(string) row) {
      res->add("<tr>");
      foreach(row, string cell)
	res->add("<td bgcolor='", cell_color, "'>", cell, "</td>");
      res->add("</tr>");
    }

  return wrap_simple_obox((string)res, frame_color, width, padding);
}

private string wrap_simple_obox( string rows, void|string frame_color,
				 void|string width, void|string padding ) {
  if(!frame_color) frame_color = "#000000";
  return "<table bgcolor='" + frame_color + "' cellspacing='0' cellpadding='0' border='0'><tr><td>\n"
    "<table bgcolor='" + frame_color + "' cellspacing='" + (width||"1") + "' cellpadding='" +
    (padding||"3") + "' border='0'>\n" + rows + "</table></td></tr></table>";
}

//! Pads out the rows in a array of rows to equal length. The new elements in
//! the rows will have the value provided in @[padding], or "&nbsp;".
array(array(string)) pad_rows( array(array(string)) rows, void|string padding ) {
  int m = max( @map(rows, sizeof) );
  if(!padding) padding = "&nbsp;";
  for(int i; i<sizeof(rows); i++)
    if(sizeof(rows[i])<m)
      rows[i] = rows[i] + allocate(m-sizeof(rows[i]), padding);
  return rows;
}

//! Provides the same functionality as the @[simple_obox] function,
//! in a "streaming" way. The real gain is different addtition methods
//! as well as the possibility to change the cell callback at any time.
//!
//! @seealso
//!   simple_obox
class OBox {

  static {
    string frame_color;
    string cell_color = "#ffffff";
    string width;
    string padding;

    int x;
    int y;

    array(array(string)) rows = ({ ({}) });

    function(int, int, string, string : string) cb;
    mapping(string:string)|array(mapping(string:string)) args;
  }

  //! @decl void create(void|string frame_color, void|string cell_color,@
  //!   void|string width, void|string padding,@
  //!   void|function(int, int, string, string : string) cell_callback)
  void create( void|string _frame_color, void|string _cell_color,
	       void|string _width, void|string _padding,
	       void|function(int, int, string, string : string) _cb) {
    if(_frame_color) frame_color = _frame_color;
    if(_cell_color) cell_color = _cell_color;
    if(_width) width = _width;
    if(_padding) padding = _padding;
    if(_cb) cb = _cb;
  }

  //! @decl void set_cell_callback(@
  //!   function(int, int, string, string : string) cell_callback)
  void set_cell_callback( function(int, int, string, string : string) _cb ) {
    cb = _cb;
  }

  //! @decl void set_extra_args( mapping(string:string) extra_args )
  //! The argument in the mapping will be added to all created table cells.

  //! @decl void set_extra_args( array(mapping(string:string)) extra_args )
  //! The argument in the mappings will be added to the cell in the
  //! cooresponding column of the table.

  void set_extra_args( mapping(string:string)|array(mapping(string:string)) _args) {
    if(mappingp(_args)) {
      if(!_args->bgcolor) _args->bgcolor = cell_color;
    }
    else
      foreach([array(mapping(string:string))]_args, mapping m)
	if(!m->bgcolor) m->bgcolor = cell_color;

    args = _args;
  }

  //! Adds this cell to the table unmodified, e.g. it should have an enclosing
  //! td or th element.
  void add_raw_cell( string cell ) {
    rows[-1] += ({ cell });
    x++;
  }

  //! Creates a cell from the provided arguments and adds it to the table.
  //!
  //! @param tag
  //!   The name of the element that should be produces. Typically
  //!   "td" or "th".
  //! @param args
  //!   A mapping with the elements attributes.
  //! @param contents
  //!   The element contents.
  void add_tagdata_cell( string tag, mapping(string:string) args, string contents ) {
    if(!args->bgcolor) args->bgcolor = cell_color;
    rows[-1] += ({ sprintf("<%s%{ %s='%s'%}>%s</%[0]s>",
			   tag, (array)args, contents) });
  }

  //! Adds a cell with the provided content.
  void add_cell( string contents ) {
    if(cb)
      rows[-1] += ({ cb(x, y, cell_color, contents) });
    else if(args && mappingp(args))
      add_tagdata_cell( "td", [mapping(string:string)]args, contents );
    else if(args && sizeof(args)>x)
      add_tagdata_cell( "td", args[x], contents );
    else
      rows[-1] += ({ "<td bgcolor='" + cell_color + "'>" +
		     contents + "</td>" });
    x++;
  }

  //! Begin a new row. Succeeding cells will be added to this
  //! row instead of the current.
  void new_row() {
    x = 0;
    y++;
    rows += ({ ({}) });
  }

  //! Adds a complete row. If the current row is nonempty a
  //! new row will be started.
  void add_row( array(string) cells ) {
    if(sizeof(rows[-1]))
      new_row();
    foreach(cells, string cell)
      add_cell( cell );
  }

  //! Ensures that all rows have the same number of cells.
  void pad_rows() {
    rows = global::pad_rows(rows, "<td bgcolor='" + cell_color + "'>&nbsp;</td>");
  }

  //! Returns the result.
  string render() {
    return wrap_simple_obox( "<tr>" + map(rows, `*, "")*"</tr><tr>\n" + "</tr>",
			     frame_color, width, padding );
  }

  //! It is possible to case this object to a string, which does the same
  //! as calling @[render], and to an array, which returns the cells in an
  //! array of rows.
  mixed cast(string to) {
    if(to=="array")
      return rows;
    if(to=="string")
      return render();
    error("Could not cast OBox object to %s.\n", to);
  }
}
