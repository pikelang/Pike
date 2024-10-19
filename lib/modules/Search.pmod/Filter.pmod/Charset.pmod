#pike __REAL_VERSION__

constant contenttypes = ({});

string low_decode_charset( string data, string charset )
{
  switch( replace(lower_case( charset ),"-","_")-"_" )
  {
    case "iso88591":
      return data;
    case "ucs2":
    case "unicode":
      return unicode_to_string( data );
      break;
    case "utf8":
      return utf8_to_string( data );

    case "euc":
    case "xeuc":
      /* should default depending on domain */
    case "eucjp":
    case "eucjapan":
    case "japanese":
      return Charset.decoder( "euc-jp" )->feed( data )->drain();

    case "xsjis":
    case "shiftjis":
    case "jis":
      return Charset.decoder("Shift_JIS")->feed(data)->drain();

    default:
      catch {
	return Charset.decoder( charset )->feed( data )->drain();
      };
      werror("\n****** Warning: Unknown charset: '"+charset+"'\n");
      return data;
  }
}

string decode_charset( string data, string charset )
{
  mixed err = catch {
    return low_decode_charset(data, charset);
  };
  werror("\n****** Warning: Invalid character encoding: %s",
	 describe_error(err));
  return data;
}

string decode_http( string data, mapping headers,
		    string default_charset )
{
  // 1: Is there a Content-type location with a charset specifier?
  string ct;
  if( (ct = headers["content-type"])
      && sscanf( ct, "%*scharset=%[^;]", ct ) == 2 )
      return decode_charset( data,  String.trim( ct ) );

  // 2: Find <meta> header in the first Kb of data.
  int done;
  zero do_meta(Parser.HTML p, mapping m)
  {
    if( done ) return 0;
    if( (lower_case(m->name||"")=="content-type")         ||
	(lower_case(m["http-equiv"]||"")=="content-type") ||
	(lower_case(m["httpequiv"]||"")=="content-type")  ||
	(lower_case(m["http-equiv"]||"")=="contenttype")  ||
	(lower_case(m["httpequiv"]||"")=="contenttype") )
    {
      if( (ct = m->content||m->data)
	  && sscanf( ct, "%*scharset=%[^;]", ct ) == 2 )
      {
        data=decode_charset( data, String.trim( ct ));
	done=1;
      }
    }
  };

  Parser.HTML p = Parser.HTML()->add_tag( "meta", do_meta );
  p->ignore_unknown(1);
  p->match_tag(0);
  p->case_insensitive_tag(1);
  p->feed( data[..1024] );
  p->finish();

  if( done )
    return data;

  // 3: DWIM.

  // 3.1: \0 here and there -> ucs2..
  if( sizeof(data[..40]/"\0") > 5 )
    return unicode_to_string( data );

  // 3.2: iso-2022, probably
  if( sizeof(data[..100]/"\33$") > 1 )
    return Charset.decoder( "iso-2022-jp")->feed( data )->drain();

  return default_charset ? decode_charset( data, default_charset ) : data;
}
