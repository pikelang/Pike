#pike __REAL_VERSION__
#pragma strict_types

//! Support for parsing PEM-style messages, defined in RFC1421.
//! Encapsulation defined in RFC934.

//! Represents a PEM-style message.
class Message
{
  //! Pre-encapsulation boundary string.
  string pre;

  //! Post-encapsulation boundary string.
  string post;

  //! Encapsulated headers. If headers occurred multiple times, they
  //! will be concatenated to the value with a null character as
  //! delimiter.
  mapping(string:string) headers;

  //! The decode message body.
  string body;

  protected void create(string|array(string) data)
  {
    array(string) lines;
    if(stringp(data))
    {
       lines = data/"\n";
       while(sizeof(lines) && lines[-1]=="")
         lines = lines[..<1];
    }
    else
      lines = [array(string)]data;

    if( sscanf(lines[0], "-----BEGIN %s-----", pre)!=1 )
      return;
    lines = lines[1..];
    if( sscanf(lines[-1], "-----END %s-----", post)==1 )
    {
      lines = lines[..<1];
      // if(pre!=post) error("Encapsulation boundary mismatch.\n");
    }

    array res = MIME.parse_headers(lines*"\n");
    headers = [mapping(string:string)]res[0];
    body = MIME.decode_base64([string]res[1]);
  }
}

//! The Messages class acts as an envelope for a PEM message file or
//! stream.
class Messages
{
  //! The fragments array contains the different message fragments, as
  //! Message objects for decoded messages and strings for
  //! non-messages or incomplete messages.
  array(string|Message) fragments = ({});

  //! This is a mapping from encapsulation boundary string to Message
  //! object. If several messages with the same boundary string are
  //! decoded, only the latest will be in this mapping. All message
  //! objects will be listed, in order, in @[fragments].
  mapping(string:Message) parts = ([]);

  //! A Messages object is created with the file or stream data.
  protected void create(string data)
  {
    array(string) current = ({});
    foreach(data/"\n";; string line)
    {
      if( has_prefix(line, "-----BEGIN ") )
      {
        // RFC1421 section 4.4p3 allows for multiple pre-EB
        fragments += process(current);
        current = ({ line });
      }
      else if( has_prefix(line, "-----END ") )
      {
        current += ({ line });
        fragments += process(current);
        current = ({});
      }
      else
        current += ({ line });
    }
    if(sizeof(current))
      fragments += process(current);

    foreach(fragments;; string|Message part)
      if(objectp(part))
      {
        Message msg = [object(Message)]part;
        parts[msg->pre]=msg;
      }
  }

  protected array(string|Message) process(array(string) lines)
  {
    if( !sizeof(lines) || !has_prefix(lines[0], "-----BEGIN ") )
      return ({ lines*"\n" });
    return ({ Message(lines) });
  }
}

//! Creates a PEM message.
//!
//! @param tag
//!   The encapsulation boundary string.
//!
//! @param data
//!   The data to be encapsulated.
//!
//! @param headers
//!   Optional mapping containing encapsulated headers as name value
//!   pairs.
string build(string tag, string data,
             void|mapping(string:string) headers)
{
  String.Buffer out = String.Buffer();
  out->add("-----BEGIN ", tag, "-----\n");
  if(headers)
  {
    foreach(headers; string name; string value)
      out->add(name, ": ", value, "\n");
    out->add("\n");
  }
  out->add(MIME.encode_base64(data), "\n-----END ", tag, "-----\n");
  return (string)out;
}
