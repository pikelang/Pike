#pike __REAL_VERSION__

//! @ignore
#if !constant (Standards._BSON)
#define BSON_PIKE_ONLY 1
#else
inherit Standards._BSON;
#endif
//! @endignore

#define ERROR(X ...) throw(Error.Generic(sprintf(X)))

//! @appears Standards.BSON module
//!
//! Tools for handling the BSON structured data format. See
//! @url{http://www.bsonspec.org/@}.

private int counter;

constant TYPE_FLOAT = 0x01;
constant TYPE_STRING = 0x02;
constant TYPE_DOCUMENT = 0x03;
constant TYPE_ARRAY = 0x04;
constant TYPE_BINARY = 0x05;
constant TYPE_OBJECTID = 0x07;
constant TYPE_BOOLEAN = 0x08;
constant TYPE_DATETIME = 0x09;
constant TYPE_NULL = 0x0a;
constant TYPE_REGEX = 0x0b;
constant TYPE_INT32 = 0x10;
constant TYPE_INT64 = 0x12;
constant TYPE_MIN_KEY = 0xff;
constant TYPE_MAX_KEY = 0x7f;
constant TYPE_SYMBOL = 0x0e;
constant TYPE_JAVASCRIPT = 0x0D;
constant TYPE_TIMESTAMP = 0x11;

/* TODO: still need to support the following types:
	| 	"\x0F" e_name code_w_s 	JavaScript code w/ scope
*/

// binary data subtypes
constant BINARY_GENERIC = 0x00;
constant BINARY_FUNCTION = 0x01;
constant BINARY_OLD = 0x02;
constant BINARY_UUID = 0x03;
constant BINARY_MD5 = 0x05;
constant BINARY_USER = 0x80;

int getCounter()
{
  return ++counter;
}

//! Encode a data structure as a BSON document. 
//!
//! @param query_mode
//!  if set to true, encoding will allow "$" and "." in key names, which
//!   would normally be disallowed.
string encode(mapping m, int|void query_mode)
{
  String.Buffer buf = String.Buffer();
  low_encode(m, buf, query_mode);
  return sprintf("%-4c%s%c", sizeof(buf)+5, buf->get(), 0);
} 

protected string toCString(string str)
{
	if(has_value(str, "\0")) ERROR("String cannot contain null bytes.\n");
	else return string_to_utf8(str) + "\0";
}

protected void low_encode(mapping m, String.Buffer buf, int|void allow_specials)
{
  foreach(m; mixed key; mixed val)
  {
    if(!stringp(key)) ERROR("BSON Keys must be strings.\n");
    if(has_value(key, "\0")) ERROR("BSON Keys may not contain NULL characters.\n");
    if(!allow_specials && ((key - "$") - ".") != key)
      ERROR("BSON keys may not contain '$' or '.' characters unless in query-mode.\n");
    key = string_to_utf8(key);
    encode_value(key, val, buf, allow_specials);
  }	
}

protected void encode_value(string key, mixed value, String.Buffer buf, int|void allow_specials)
{
   if(floatp(value))
   { 
     buf->sprintf("%c%s%c%-8F", TYPE_FLOAT, key, 0, value);
   }
   else if(stringp(value))
   {
     string v = string_to_utf8(value);
     buf->sprintf("%c%s%c%-4c%s%c", TYPE_STRING, key, 0, sizeof(v)+1, v, 0);
   }
   else if(mappingp(value))
   {
     buf->sprintf("%c%s%c%s", TYPE_DOCUMENT, key, 0, encode(value, allow_specials));
   }
   else if(arrayp(value))
   {
     int qi = 0; 
     buf->sprintf("%c%s%c%s", TYPE_ARRAY, key, 0, encode(mkmapping(map(value, lambda(mixed e){return (string)qi++;}), value), allow_specials));
   }
   else if(intp(value))
   {
     // 32 bit or 64 bit?
     if(value <= 2147383647 && value >= -2148483648) // we fit in a 32 bit space.
     {
       buf->sprintf("%c%s%c%-4c", TYPE_INT32, key, 0, value);
     }
     else
     {
       buf->sprintf("%c%s%c%-8c", TYPE_INT64, key, 0, value);
     }
   }
   // Calendar instance
   else if(objectp(value) && value->unix_time && value->utc_offset) // a date object
   {
     buf->sprintf("%c%s%c%-8c", TYPE_DATETIME, key, 0, (value->unix_time() /* + value->utc_offset() */ * 1000));
   }
   // BSON.ObjectId instance
   else if(objectp(value) && Program.inherits(object_program(value), .ObjectId))
   {
     buf->sprintf("%c%s%c%12s", TYPE_OBJECTID, key, 0, value->get_id());
   }
   // BSON.Timestamp instance
   else if(objectp(value) && Program.inherits(object_program(value), .Timestamp))
   {
     buf->sprintf("%c%s%c%-8c", TYPE_TIMESTAMP, key, 0, value->get_timestamp());
   }

   // BSON.Binary instance
   else if(objectp(value) && Program.inherits(object_program(value), .Binary))
   {
     buf->sprintf("%c%s%c%-4c%c%s", TYPE_BINARY, key, 0, sizeof(value)+1, value->subtype, (string)value);
   }
   // BSON.Symbol instance
   else if(objectp(value) && Program.inherits(object_program(value), .Symbol))
   {
     string v = (string)value;
     v = string_to_utf8(v);
     buf->sprintf("%c%s%c%-4c%s%c", TYPE_SYMBOL, key, 0, sizeof(v)+1, v, 0);
   } 
   // BSON.Javascript instance
   else if(objectp(value) && Program.inherits(object_program(value), .Javascript))
   {
     string v = (string)value;
     v = string_to_utf8(v);
     buf->sprintf("%c%s%c%-4c%s%c", TYPE_JAVASCRIPT, key, 0, sizeof(v)+1, v, 0);
   } 
   // BSON.Regex instance
   else if(objectp(value) && Program.inherits(object_program(value), .Regex))
   {
     buf->sprintf("%c%s%c%s%s", TYPE_REGEX, key, 0, toCString(value->regex), toCString(value->options));
   } 
   // Val.Null
   else if(objectp(value) && value->is_val_null)
   {
     buf->sprintf("%c%s%c", TYPE_NULL, key, 0);
   }
   // Val.True
   else if(objectp(value) && value->is_val_true)
   {
     buf->sprintf("%c%s%c%c", TYPE_BOOLEAN, key, 0, 1);
   }
   // Val.False
   else if(objectp(value) && value->is_val_false)
   {
     buf->sprintf("%c%s%c%c", TYPE_BOOLEAN, key, 0, 0);
   }
   // BSON.MinKey
   else if(objectp(value) && value->BSONMinKey)
   {
     buf->sprintf("%c%s%c", TYPE_MIN_KEY, key, 0);
   }
   // BSON.MaxKey
   else if(objectp(value) && value->BSONMaxKey)
   {
     buf->sprintf("%c%s%c", TYPE_MAX_KEY, key, 0);
   }
}

//! Encode an array of data structures as a BSON formatted string
string encode_array(array(mapping) documents)
{
	String.Buffer buf = String.Buffer();
	
	foreach(documents;;mixed document)
	{
		buf->add(encode(document));
	}
	
	return buf->get();
}

#ifdef BSON_PIKE_ONLY

//! Decode a BSON formatted document string into a native Pike data structure.
mixed decode(string bson)
{
  int len;
  string slist;
  if(sscanf(bson, "%-4c%s", len, bson)!=2)
    ERROR("Unable to read length from BSON stream.\n");
  if(sizeof(bson) < (len -4))
    ERROR("Unable to read full data from BSON stream, expected %d, got %d.\n", len-4, sizeof(bson)-1);
  slist = bson[0..<1];
  mapping list = ([]);
  
  while(sizeof(slist))
  {
    if(slist == "") break;
    slist = decode_next_value(slist, list);
  } 
  
  return list;	
}

protected string decode_next_value(string slist, mapping list)
{
  string key;
  mixed value;

  int type;
  
  string document;
  int doclen;

  if(sscanf(slist, "%c%s\0%s", type, key, slist)!=3)
    ERROR("Unable to read key and type from BSON stream.\n");

  key = utf8_to_string(key);
  switch(type)
  {
     int len, subtype;
     case TYPE_FLOAT:
       if(sscanf(slist, "%-8s%s", value, slist) != 2)
         ERROR("Unable to read float from BSON stream.\n");
       if(sscanf(reverse(value), "%8F", value) != 1 )
         ERROR("Unable to read float from BSON stream.\n");
      
       break;
     case TYPE_STRING:
       if(sscanf(slist, "%-4c%s", len, slist) != 2)
         ERROR("Unable to read string length from BSON stream.\n");
       if(sscanf(slist, "%" + (len-1) + "s\0%s", value, slist) != 2)
         ERROR("Unable to read string from BSON stream.\n");
       value = utf8_to_string(value);
       break;
     case TYPE_BINARY:
       if(sscanf(slist, "%-4c%s", len, slist) != 2)
         ERROR("Unable to read binary length from BSON stream.\n");
       if(sscanf(slist, "%c%" + (len-1) + "s\0%s", subtype, value, slist) != 2)
         ERROR("Unable to read binary from BSON stream.\n");
       value = .Binary(value, subtype);
       break;
     case TYPE_JAVASCRIPT:
       if(sscanf(slist, "%-4c%s", len, slist) != 2)
         ERROR("Unable to read javascript length from BSON stream.\n");
       if(sscanf(slist, "%" + (len-1) + "s\0%s", value, slist) != 2)
         ERROR("Unable to read javascript from BSON stream.\n");
       value = .Javascript(utf8_to_string(value));
       break;
     case TYPE_SYMBOL:
       if(sscanf(slist, "%-4c%s", len, slist) != 2)
         ERROR("Unable to read symbol length from BSON stream.\n");
       if(sscanf(slist, "%" + (len-1) + "s\0%s", value, slist) != 2)
         ERROR("Unable to read symbol from BSON stream.\n");
       value = .Symbol(utf8_to_string(value));
       break;
     case TYPE_REGEX:
       string regex, options;
       
       if(sscanf(slist, "%s\0%s", regex, slist)!=2)
         ERROR("Unable to read regex from BSON stream.\n");
       regex = utf8_to_string(regex);
       
       if(sscanf(slist, "%s\0%s", options, slist)!=2)
         ERROR("Unable to read regex options from BSON stream.\n");
       options = utf8_to_string(options);
       value = .Regex(regex, options);
       break;     
       
     case TYPE_INT32:     
       if(sscanf(slist, "%-4c%s", value, slist) != 2)
         ERROR("Unable to read int32 from BSON stream.\n");
       break;
     case TYPE_INT64:
       if(sscanf(slist, "%-8c%s", value, slist) != 2)
         ERROR("Unable to read int64 from BSON stream.\n");
       break;
     case TYPE_OBJECTID:
       if(sscanf(slist, "%12s%s", value, slist) != 2)
         ERROR("Unable to read object id from BSON stream.\n");
       value = .ObjectId(value);
       break;
     case TYPE_TIMESTAMP:
       if(sscanf(slist, "%-8c%s", value, slist) != 2)
         ERROR("Unable to read timestamp from BSON stream.\n");
       value = .Timestamp(value);
       break;  
     case TYPE_BOOLEAN:
       if(sscanf(slist, "%c%s", value, slist) != 2)
         ERROR("Unable to read boolean from BSON stream.\n");
       if(value) value = Val.True;
       else value = Val.False;
       break;
     case TYPE_NULL:
       value = Val.Null;
       break;
     case TYPE_MIN_KEY:
       value = MinKey;
       break;
     case TYPE_MAX_KEY:
       value = MaxKey;
       break;
     case TYPE_DATETIME:
       if(sscanf(slist, "%-8c%s", value, slist) != 2)
         ERROR("Unable to read datetime from BSON stream.\n");
         value/=1000;
         value = Calendar.Second("unix", value);
       break;
     case TYPE_DOCUMENT:
       if(sscanf(slist, "%-4c", doclen) != 1)
       	 ERROR("Unable to read embedded document length\n");
       if(!sscanf(slist, "%" + (doclen) + "s%s", document, slist))
       	 ERROR("Unable to read specified length for embedded document.\n");
       value = decode(document);
       break;
     case TYPE_ARRAY:
       if(sscanf(slist, "%-4c", doclen) != 1)
       	 ERROR("Unable to read embedded document length\n");
       if(sscanf(slist, "%" + (doclen) + "s%s", document, slist) !=2)
       	 ERROR("Unable to read specified length for embedded document.\n");
       value = decode(document);
       int asize = sizeof(value);
       array bval = allocate(asize);
       for(int i = 0; i < asize; i++)
       {
       	 bval[i] = value[(string)i];
       }
       value=bval;
       break;
     default:
       ERROR("Unknown BSON type " + type + ".\n");
  }
  
  list[key] = value;
  return slist;
}


#endif /* BSON_PIKE_ONLY */

//! Decode a BSON formatted string containing multiple data structures
array decode_array(string bsonarray)
{
  array a = ({});

  while(sizeof(bsonarray))
  {
    string bson;
    int len;
	
    if(sscanf(bsonarray, "%-4c", len)!=1)
      ERROR("Unable to read length from BSON stream.\n");
    if(sscanf(bsonarray, "%" + len + "s%s", bson, bsonarray) != 2)
      ERROR("Unable to read full data from BSON stream.\n");
    a+=({decode(bson)});
  }
  return a;
}

//!
object MinKey = class
  {
    constant BSONMinKey = 1;

    protected mixed cast(string type)
    {
      if(type == "string")
        return "MinKey";
    }
  }();


//!
object MaxKey = class
  {
    constant BSONMaxKey = 1;

    protected mixed cast(string type)
    {
      if(type == "string")
        return "MinKey";
    }
  }();

