#pike __REAL_VERSION__

constant verify_internals = _verify_internals;
constant memory_usage = _memory_usage;
constant gc_status = _gc_status;
constant describe_program = _describe_program;
constant size_object = _size_object;
constant map_all_objects = _map_all_objects;

#if constant(_debug)
// These functions require --with-rtldebug.
constant debug = _debug;
constant optimizer_debug = _optimizer_debug;
constant assembler_debug = _assembler_debug;
constant disassemble = Builtin._disassemble;
constant dump_program_tables = _dump_program_tables;
constant locate_references = _locate_references;
constant describe = _describe;
constant gc_set_watch = _gc_set_watch;
constant dump_backlog = _dump_backlog;
#endif

#if constant(_dmalloc_set_name)
// These functions require --with-dmalloc.
constant reset_dmalloc = _reset_dmalloc;
constant dmalloc_set_name = _dmalloc_set_name;
constant list_open_fds = _list_open_fds;
constant dump_dmalloc_locations = _dump_dmalloc_locations;
#endif

#if constant(_compiler_trace)
// Requires -DYYDEBUG.
constant compiler_trace  = _compiler_trace;
#endif

/* significantly more compact version of size2string */
private string mi2sz( int i )
{
    constant un = ({ "", "k","M","G","T" });
    float x = (float)i;
    int u = 0;
    do
    {
        x /= 1024.0;
        u++;
    } while( x > 1000.0 );
    return sprintf("%.1f%s",x,un[u]);
}

//! Returns a pretty printed version of the output from
//! @[count_objects] (with added estimated RAM usage)
string pp_object_usage()
{
    mapping tmp = ([]);
    int num;
    int total = map_all_objects( lambda( object start )
       {
           num++;
           program q = object_program( start );
           string nam = sprintf("%O",(program)q);
           int memsize = size_object(start);
           sscanf( nam, "object_program(%s)", nam );

           if( !tmp[nam] )
           {
               tmp[nam] = ({1,memsize,Program.defined(q)});
           }
           else
           {
               tmp[nam][0]++;
               tmp[nam][1]+=memsize;
           }
       });

    array by_size = ({});
    foreach( tmp; string obj; array(int) cnt )
    {
        /* some heuristics: Do not show singletons unless it is using a noticeable amount of RAM. */
        if( cnt[1] * cnt[0] < 2048 )
            continue;
        by_size += ({ ({ cnt[1], obj, cnt[0], cnt[2] }) });
    }
    by_size += ({ ({ (total-num)*32, "destructed", (total-num), "-" }) });

    sort( by_size );
    string res = "";
    res += sprintf("-"*(33+6+9+1+21)+"\n"+
                   "%-33s %6s %8s %20s\n"+
                   "-"*(33+6+9+1+21)+"\n",
                   "Object", "Clones","Memory", "Loc");

    foreach( reverse(by_size), [int size, string obj, int cnt, string p] )
        res += sprintf( "%-33s %6d %8s %20s\n", obj[<32..], cnt, mi2sz(size), basename(p||"-"));

    res += ("-"*(33+6+9+1+21)+"\n\n");
    return res;
}

//! Returns a pretty printed version of the
//! output from @[memory_usage].
string pp_memory_usage() {
    string ret="             Num   Bytes\n";
    mapping q = _memory_usage();
    mapping res = ([]), malloc = ([]);
    foreach( q; string key; int val )
    {
        mapping in = res;
        string sub ;
        if( has_suffix( key, "_bytes" ) )
        {
            sub = "bytes";
            key = key[..<6]+"s";
            if( key == "mallocs" )
                key = "malloc";
        }
        if( has_prefix( key, "num_" ) )
        {
            sub = "num";
            key = key[4..];
        }
        if( key == "short_pike_strings" )
            key = "strings";

        if(key == "free_blocks" || key == "malloc" || key == "malloc_blocks" )
            in = malloc;
        if( val )
        {
            if( !in[key] )
                in[key] = ([]);
            in[key][sub] += val;
        }
    }

    string output_one( mapping res, bool trailer )
    {
        array output = ({});
        foreach( res; string key; mapping what )
            output += ({({ what->bytes, what->num, key })});
        output = reverse(sort( output ));

        string format_row( array x )
        {
            return sprintf("%-20s %10s %10d\n", x[2], mi2sz(x[0]), x[1] );
        };

        return
            sprintf("%-20s %10s %10s\n", "Type", "Bytes", "Num")+
            "-"*(21+11+10)+"\n"+
            map( output, format_row )*""+
            "-"*(21+11+10)+"\n"+
            (trailer?
             format_row( ({ Array.sum( column(output,0) ), Array.sum( column(output,1) ), "total" }) ):"");
    };
    return output_one(res,true) + 
        "\nMalloc info:\n"+output_one(malloc,false);
}

//! Returns the number of objects of every kind in memory.
mapping(string:int) count_objects()
{
  mapping(string:int) ret = ([]);
  int num, total;
  total = map_all_objects(lambda(object obj ) {
    string p = sprintf("%O",object_program(obj));
    sscanf(p, "object_program(%s)", p);
    ret[p]++;
    num++;
  });
  if( total > num )
      ret["destructed"] = total-num;
  return ret;
}

//
// Stuff related to the encode_value decoder.
//

protected enum DecoderFP {
  FP_SNAN = -4,
  FP_QNAN = -3,
  FP_NINF = -2,
  FP_PINF = -1,
  FP_ZERO = 0,
  FP_PZERO = 0,
  FP_NZERO = 1,
}

protected enum DecoderTags {
  TAG_ARRAY = 0,
  TAG_MAPPING = 1,
  TAG_MULTISET = 2,
  TAG_OBJECT = 3,
  TAG_FUNCTION = 4,
  TAG_PROGRAM = 5,
  TAG_STRING = 6,
  TAG_FLOAT = 7,
  TAG_INT = 8,
  TAG_TYPE = 9,           /* Not supported yet */

  TAG_DELAYED = 14,	/* Note: Coincides with T_ZERO. */
  TAG_AGAIN = 15,

  TAG_NEG = 16,
  TAG_SMALL = 32,
}

protected enum EntryTypes {
  ID_ENTRY_TYPE_CONSTANT	= -4,
  ID_ENTRY_EFUN_CONSTANT	= -3,
  ID_ENTRY_RAW			= -2,
  ID_ENTRY_EOT			= -1,
  ID_ENTRY_VARIABLE		= 0,
  ID_ENTRY_FUNCTION		= 1,
  ID_ENTRY_CONSTANT		= 2,
  ID_ENTRY_INHERIT		= 3,
  ID_ENTRY_ALIAS		= 4,
}

protected enum PikeTypes {
  T_INT = 0,
  T_FLOAT = 1,

  T_ZERO = 6,

  T_ARRAY = 8,
  T_MAPPING = 9,
  T_MULTISET = 10,
  T_OBJECT = 11,
  T_FUNCTION = 12,
  T_PROGRAM = 13,
  T_STRING = 14,
  T_TYPE = 15,
  T_VOID = 16,
  T_MANY = 17,

  T_ATTRIBUTE = 238,
  T_NSTRING = 239,
  T_NAME = 241,
  T_SCOPE = 243,
  T_ASSIGN = 245,
  T_UNKNOWN = 247,
  T_MIXED = 251,
  T_NOT = 253,
  T_AND = 254,
  T_OR = 255,
}

protected class Decoder(Stdio.Buffer input)
{
  mapping(int:mixed) decoded = ([]);
  int counter = -(1<<(8-6));
  int depth = 0;

  int num_errors;

  void decode_error(strict_sprintf_format fmt, sprintf_args ... args)
  {
    num_errors++;
    werror("ERROR: %s", sprintf(fmt, @args));
  }

  array(int) next()
  {
    int what = input->read_int(1);
    int e = what >> 6;
    int num;
    if(what & TAG_SMALL) {
      num = e;
    } else {
      num = 0;
      while(e-->=0) {
	num = (num<<8) + (input->read_int(1)+1);
      }
      num += (1<<(8-6)) - 1;
    }
    if(what & TAG_NEG) {
      num = ~num;
    }
    return ({ what & 0x0f, num });
  }

  protected int decode_number()
  {
    int what, num;
    [what, num] = next();
    return what | (num << 4);
  }

  protected string get_string_data(int len)
  {
    int shift = 0;

    if((len) == -1) {
      [shift, len] = next();
      if(shift<0 || shift>2)
	decode_error ("Illegal size shift %d.\n", shift);
      len = len << shift;
    }
    if (len < 0) {
      decode_error("Illegal negative size %d.\n", len);
      return "";
    }
    if (len > sizeof(input)) {
      decode_error("Too large size %td (max is %td).\n",
		   len, sizeof(input));
      len = sizeof(input);
    }
    string ret = input->read(len);

    if (shift) {
      // FIXME: Only shift 1!
      ret = unicode_to_string(ret);
    }
    return ret;
  }

  protected mixed decode_type()
  {
    int tmp = input->read_int(1);
    if (tmp <= 15) tmp ^= 8;

    switch(tmp)
    {
    default:
      decode_error("Error in type string (%d).\n", tmp);
      break;

    case T_ASSIGN:
      tmp = input->read_int(1);
      if ((tmp < '0') || (tmp > '9')) {
	decode_error("Bad marker in type string (%d).\n", tmp);
      }
      return decode_type();

    case T_SCOPE:
      tmp = input->read_int(1);
      return decode_type();

    case T_FUNCTION:
      int narg = 0;

      while (input->read_int(1) != T_MANY) {
	input->unread(1);
	decode_type();
      }
      decode_type();	/* Many */
      decode_type();	/* Return */
      return typeof(decode_type);

    case T_MAPPING:
    case T_OR:
    case T_AND:
      decode_type();
      return decode_type();

    case T_TYPE:
    case T_PROGRAM:
    case T_ARRAY:
    case T_MULTISET:
    case T_NOT:
      return decode_type();

    case T_INT:
      input->read_int(4);	// min
      input->read_int(4);	// max
      return typeof([int](mixed)0);

    case T_STRING:
      return typeof([string](mixed)0);

    case T_NSTRING:
      decode_type();
      return typeof([string](mixed)0);

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case T_FLOAT:
    case T_MIXED:
    case T_ZERO:
    case T_VOID:
    case T_UNKNOWN:
      return typeof((mixed)0);

    case T_ATTRIBUTE:
      werror("%*sType attribute:\n",
	     depth, "");
      depth += 2;
      low_decode();
      depth -= 2;

      return decode_type();

    case T_NAME:
      werror("%*sType name:\n",
	     depth, "");
      depth += 2;
      low_decode();
      depth -= 2;

      return decode_type();

    case T_OBJECT:
      int flag = input->read_int(1);
      werror("%*sObject type (%d):\n",
	     depth, "", flag);
      depth += 2;
      low_decode();
      depth -= 2;
      return typeof(this);
    }

    return typeof((mixed)0);
  }

  void decode_reference()
  {
    int id_flags = decode_number();

    werror("%*sid_flags: 0x%04x\n"
	   "%*sname:\n",
	   depth, "", id_flags,
	   depth, "");

    depth += 2;

    low_decode();

    werror("%*stype:\n",
	   depth - 2, "");
    low_decode();

    depth -= 2;

    /* filename */
    int filename_strno = decode_number();
    werror("%*sfilename strno: %d\n",
	   depth, "", filename_strno);

    int linenumber = decode_number();
    werror("%*sline number: %d\n",
	   depth, "", linenumber);
  }

  int low_decode()
  {
    int entry_id;

    [int what, int num] = next();

    mixed delayed_enc_val;

    switch(what)
    {
    case TAG_DELAYED:
      werror("%*sDecoding delay encoded from <%d>\n",
	     depth, "", num);
      if (zero_type(delayed_enc_val = decoded[num])) {
	decode_error("Failed to find previous record of "
		     "delay encoded entry <%d>.\n", num);
      }
      [what, num] = next();
      break;

    case TAG_AGAIN:
      break;

    default:
      entry_id = counter++;
      /* Fall through. */

    case TAG_TYPE:
      werror("%*sDecoding to <%d>: TAG%d (%d)\n",
	     depth, "", entry_id,
	     what, num);
      /* Types are added to the encoded mapping AFTER they have been
       * encoded. */
      delayed_enc_val = UNDEFINED;
      break;
    }

    mixed val = UNDEFINED;

    switch(what)
    {
    case TAG_AGAIN:
      werror("%*sDecoding TAG_AGAIN from <%d>\n",
	     depth, "", num);
      entry_id = num;
      val = decoded[num];
      if (!zero_type(val)) {
	werror("%*s  %O\n",
	       depth, "", val);
      } else {
	decode_error("Failed to decode TAG_AGAIN entry <%d>.\n", num);
      }
      break;

    case TAG_INT:
      werror("%*s  Integer: %d\n",
	     depth, "", val = num);
      break;

    case TAG_STRING:
      val = get_string_data(num);
      werror("%*s  String: %O\n",
	     depth, "", val);
      break;

    case TAG_FLOAT:
      werror("%*s  Float mantissa:0x%016x\n",
	     depth, "", num);

      val = num;

      [what, num] = next();

      werror("%*s  Float exponent: %d\n",
	     depth, "", num);

      if(!val)
      {
	switch(num)
	{
	  case FP_SNAN: /* Signal Not A Number */
	  case FP_QNAN: /* Quiet Not A Number */
	    werror("%*s    Float: nan\n",
		   depth, "");
	    val = Math.nan;
	    break;

	  case FP_NINF: /* Negative infinity */
	    werror("%*s    Float: -inf\n",
		   depth, "");
	    val = -Math.inf;
	    break;

	  case FP_PINF: /* Positive infinity */
	    werror("%*s    Float: inf\n",
		   depth, "");
	    val = Math.inf;
	    break;

	  case FP_NZERO: /* Negative Zero */
	    werror("%*s    Float: -0.0\n",
		   depth, "");
	    val = -0.0;
	    break;

	  default:
	    werror("%*s    Float: 0.0\n",
		   depth, "");
	    val = 0.0;
	    break;
	}
	break;
      }

      // FIXME: Not correct.
      val = (float)val;

      break;

    case TAG_TYPE:
      val = decode_type();
      if (val) {
	werror("%*s  Type: %O\n",
	       depth, "", val);
      }

      entry_id = counter++;
    break;

    case TAG_ARRAY:
      val = ({});

      if(num < 0) {
	decode_error("Failed to decode array (array size is negative (%d)).\n",
		     num);
	break;
      }

      /* Heuristical */
      if(num > sizeof(input)) {
	decode_error("Failed to decode array (not enough data).\n");
	break;
      }

      werror("%*sArray (%d elements) <%d>:\n",
	     depth, "", num, entry_id);

      depth += 4;

      for(int e = 0; e < num; e++)
      {
	werror("%*sElement #%d:\n",
	       depth - 2, "", e);
	low_decode();
      }

      depth -= 4;

      break;

    case TAG_MAPPING:
      val = ([]);

      if(num<0) {
	decode_error("%*sFailed to decode mapping "
		     "(mapping size is negative (%d)).\n",
		     depth, "", num);
	break;
      }

      /* Heuristical */
      if(num > sizeof(input)) {
	decode_error("Failed to decode mapping (not enough data).\n");
	break;
      }

      werror("%*sMapping (%d elements) <%d>\n",
	     depth, "", num, entry_id);

      depth += 4;

      for(int e = 0; e < num; e++)
      {
	werror("%*sIndex #%d:\n",
	       depth - 2, "", e);
	low_decode();
	werror("%*sValue #%d:\n",
	       depth - 2, "", e);
	low_decode();
      }

      depth -= 4;

      break;

    case TAG_MULTISET:
      val = ([]);

      if(num<0) {
	decode_error("Failed to decode multiset "
		     "(multiset size is negative (%d)).\n", num);
	break;
      }

      /* Heuristical */
      if(num > sizeof(input)) {
	decode_error("Failed to decode multiset "
		     "(not enough data).\n");
	break;
      }

      werror("%*sMultiset (%d elements) <%d>\n",
	     depth, "", num, entry_id);

      depth += 4;

      for(int e = 0 ; e < num; e++)
      {
	werror("%*sIndex #%d:\n",
	       depth - 2, "", e);
	low_decode();
      }

      depth -= 4;

      break;

    case TAG_OBJECT:
      val = predef::__placeholder_object;

      int subtype = 0;
      if (num == 4) {
	subtype = decode_number();
      }

      switch(num)
      {
	case 0:
	  werror("%*sObject OBJECTOF <%d>\n",
		 depth, "", entry_id);

	  depth += 2;

	  low_decode();

	  depth -= 2;

	  break;

	case 1:
	  werror("%*sObject CLONE_OBJECT <%d>\n",
		 depth, "", entry_id);

	  depth += 2;

	  low_decode();

	  werror("%*sObject DECODE_OBJECT <%d>\n",
		 depth - 2, "", entry_id);

	  low_decode();

	  depth -= 2;

	  break;

	case 2:
	  werror("%*sBignum BASE 36 <%d>\n",
		 depth, "", entry_id);

	  low_decode();

	  break;

	case 3:
	  werror("%*sObject DOUBLE_VALUE <%d>\n",
		 depth, "", entry_id);

	  depth += 4;

	  low_decode();

	  werror("%*sObject Second value <%d>\n",
		 depth - 2, "", entry_id);

	  low_decode();

	  depth -= 4;

	  break;

        case 4:
	  werror("%*sObject SUBTYPE %d <%d>\n",
		 depth, "", subtype, entry_id);
	  if (subtype < 0) {
	    decode_error("Negative subtype for object: %d.\n",
			 subtype);
	    break;
	  }
	  break;

	default:
	  decode_error("Object coding not compatible: %d\n", num);
	  break;
      }

      break;

    case TAG_FUNCTION:
      val = lambda(){};

      switch(num)
      {
	case 0:
	  werror("%*sFunction FUNCTIONOF <%d>\n",
		 depth, "", entry_id);

	  depth += 2;

	  low_decode();

	  depth -= 2;
	  break;

	case 1:
	  werror("%*sFunction OBJECT_FUNCTION <%d>\n",
		 depth, "", entry_id);

	  depth += 4;

	  low_decode();

	  werror("%*sFunction name <%d>\n",
		 depth, "", entry_id);

	  low_decode();

	  depth -= 4;

	  break;

	default:
	  decode_error("Function coding not compatible: %d\n", num);
	  break;
      }

      break;


    case TAG_PROGRAM:
      val = class{};

      switch(num)
      {
	case 0:
	  werror("%*sProgram PROGRAMOF <%d>\n",
		 depth, "", entry_id);

	  depth += 2;

	  low_decode();

	  depth -= 2;

	  break;

	case 1:			/* Old-style encoding. */
	  werror("%*sProgram OLD_STYLE <%d>\n",
		 depth, "", entry_id);

	  decode_error("Failed to decode program. Old-style program "
		       "encoding is not supported, anymore.\n");

	  break;

	case 2:
	  werror("%*sProgram ARROW_INDEX <%d>\n",
		 depth, "", entry_id);

	  depth += 4;

	  low_decode();

	  werror("%*sArrow index <%d>\n",
		 depth - 2, "", entry_id);

	  low_decode();

	  depth -= 4;

	  break;

        case 3:
	  werror("%*sProgram ID_TO_PROGRAM <%d>\n",
		 depth, "", entry_id);

	  depth += 2;

	  low_decode();

	  depth -= 2;
	  break;

	case 5:		/* Forward reference for new-style encoding. */
	  werror("%*sProgram FORWARD_REF <%d>\n",
		 depth, "", entry_id);
	  break;

	case 4:			/* New-style encoding. */
	  werror("%*sProgram NEW_STYLE <%d>\n",
		 depth, "", entry_id);

	  // Handle circularities.
	  decoded[entry_id] = val;

	  depth += 6;

	  int byteorder = decode_number();
	  int p_flags = decode_number();

	  werror("%*sByte-order: %d\n"
		 "%*sProgram flags: 0x%08x\n",
		 depth - 4, "", byteorder,
		 depth - 4, "", p_flags);

	  if ((byteorder != 1234) && (byteorder != 4321)) {
	    decode_error("Unsupported byte-order: %d.\n", byteorder);
	    break;
	  }

	  werror("%*sPike Version\n",
		 depth - 4, "");

	  low_decode();

	  werror("%*sParent <%d>\n",
		 depth - 4, "", entry_id);
	  low_decode();

	  array(string) fields = ({
	    "program",
	    "relocations",
	    "linenumbers",
	    "identifier_index",
	    "variable_index",
	    "strings",
	    "constants",
	    "identifier_references",
	    "inherits",
	    "identifiers",
	  });

	  mapping(string:int) lengths = ([]);

	  foreach(fields; int e; string field) {
	    lengths[field] = decode_number();
	  }

	  array(int) constants = allocate(lengths->constants);

	  int bytecode_method = decode_number();
	  switch(bytecode_method) {
	  case -1:
	    werror("%*sPortable byte code.\n",
		   depth - 4, "");
	    break;
	  default:
	    decode_error("Unsupported byte-code method: %d\n", bytecode_method);
	    break;
	  }

	  werror("%*sString table:\n",
		 depth - 4, "");
	  for (int e = 0; e < lengths->strings; e++) {
	    low_decode();
	  }

	  werror("%*sIdentifier references:\n",
		 depth - 4, "");

	  int entry_type;
	  do {
	    entry_type = decode_number();

	    int id_flags;

	    switch(entry_type) {
	    case ID_ENTRY_EOT:
	      break;

	    case ID_ENTRY_EFUN_CONSTANT:
	      int efun_no = decode_number();

	      if ((efun_no < 0) || (efun_no >= lengths->constants)) {
		decode_error("Bad efun number: %d (expected 0 - %d).\n",
			     efun_no, lengths->constants-1);
		break;
	      }

	      werror("%*sEfun #%d\n",
		     depth - 2, "", efun_no);

	      low_decode();

	      werror("%*sName\n",
		     depth - 2, "");

	      low_decode();

	      break;
	    case ID_ENTRY_TYPE_CONSTANT:
	      int type_no = decode_number();

	      if ((type_no < 0) || (type_no >= lengths->constants)) {
		decode_error("Bad type number: %d (expected 0 - %d).\n",
			     type_no, lengths->constants-1);
	      }

	      werror("%*sType #%d\n",
		     depth - 2, "", type_no);

	      low_decode();

	      werror("%*sName\n",
		     depth - 2, "");

	      low_decode();

	      break;

	    case ID_ENTRY_RAW:
	      id_flags = decode_number();

	      int inherit_offset = decode_number();

	      /* identifier_offset */
	      /* Actually the id ref number from the inherited program */
	      int ref_no = decode_number();

	      if (inherit_offset >= lengths->inherits) {
		decode_error("Inherit offset out of range %d vs %d.\n",
			     inherit_offset, lengths->inherits);
	      }
	      if (ref_no < 0) {
		decode_error("Identifier reference out of range %d.\n",
			     ref_no);
	      }

	      /* Expected identifier reference number */
	      int raw_no = decode_number();

	      if (raw_no < 0 || raw_no > lengths->identifier_references) {
		decode_error("Bad identifier reference offset: %d != %d\n",
			     raw_no,
			     lengths->identifier_references);
	      }

	      werror("%*sRaw reference\n"
		     "%*sno: %d inherit: %d id_flags: 0x%04x ref_no: %d\n",
		     depth - 2, "",
		     depth, "", raw_no, inherit_offset, id_flags, ref_no);
	      break;

	    case ID_ENTRY_VARIABLE:
	      werror("%*sVariable\n",
		     depth - 2, "");
	      decode_reference();

	      werror("%*sref_no: %d\n",
		     depth, "", decode_number());

	      break;
	    case ID_ENTRY_FUNCTION:
	      werror("%*sFunction\n",
		     depth - 2, "");
	      decode_reference();

	      int func_flags = decode_number();

	      int func_offset = decode_number();

	      /* opt_flags */
	      int opt_flags = decode_number();

	      werror("%*sfunc: %d flags: 0x%04x opt: 0x%08x ref_no: %d\n",
		     depth, "", func_offset, func_flags, opt_flags,
		     decode_number());
	      break;
	    case ID_ENTRY_CONSTANT:
	      werror("%*sConstant\n",
		     depth - 2, "");
	      decode_reference();

	      /* offset */
	      int const_no = decode_number();

	      /* run_time_type */
	      int rtt = decode_number();

	      /* opt_flags */
	      int c_opt_flags = decode_number();

	      werror("%*sconst_no: %d rtt: %d opt: 0x%08x ref_no: %d\n",
		     depth, "", const_no, rtt, c_opt_flags,
		     decode_number());
	      break;
	    case ID_ENTRY_ALIAS:
	      werror("%*sAlias\n",
		     depth - 2, "");
	      decode_reference();

	      /* depth */
	      int refto_depth = decode_number();

	      /* refno */
	      int refto = decode_number();

	      werror("%*sref_to: [%d:%d] ref_no: %d\n",
		     depth, "", refto_depth, refto, decode_number());

	      break;
	    case ID_ENTRY_INHERIT:
	      id_flags = decode_number();

	      int before = decode_number();

	      werror("%*sInherit name:\n",
		     depth - 2, "");
	      low_decode();

	      werror("%*sInherit program:\n",
		     depth - 2, "");
	      low_decode();

	      werror("%*sParent object:\n",
		     depth - 2, "");
	      low_decode();

	      /* parent_identifier */
	      int parent_id = decode_number();

	      /* parent_offset */
	      int parent_offset = decode_number();

	      /* Expected number of identifier references. */
	      int after = decode_number();

	      werror("%*sParent id: %d offset: %d added_refs: %d\n",
		     depth, "", parent_id, parent_offset, after - before);

	      break;
	    default:
	      decode_error("Unsupported id entry type: %d\n",
			   entry_type);
	    }
	  } while (entry_type != ID_ENTRY_EOT);

	  werror("%*sConstants:\n",
		 depth - 4, "");

	  for (int e = 0; e < lengths->constants; e++) {
	    if (constants[e]) continue;
	    werror("%*sConstant #%d\n",
		   depth - 2, "", e);
	    /* value */
	    low_decode();
	    /* name */
	    low_decode();
	  }

	  depth -= 6;

	  break;

	default:
	  decode_error("Cannot decode program encoding type %d\n",num);
	  break;
      }
      break;

    default:
      decode_error("Failed to restore string (illegal type %d).\n",
		   what);
      break;
    }

    decoded[entry_id] = val;
  }

  int decode()
  {
    int ret = 0;
    do {
      ret += low_decode();

      if (sizeof(input)) {
	werror("\n\n");
      }
    } while (sizeof(input));

    return ret;
  }
}

//! Describe the contents of an @[encode_value()] string.
//!
//! @returns
//!   Returns the number of encoding errors that were detected (if any).
int describe_encoded_value(string data)
{
  Stdio.Buffer input = Stdio.Buffer(data);

  string header = input->read(4);
  if ((String.width(data) != 8) || (header != "\266ke0") || !sizeof(input)) {
    werror("Not an encoded value.\n");
    return 1;
  }

  Decoder state = Decoder(input);

  return state->decode();
}
