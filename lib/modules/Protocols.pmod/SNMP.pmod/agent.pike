#pike __REAL_VERSION__

//
// agent.pike
//! A simple SNMP agent with support for SNMP Get requests
//

#include "snmp_globals.h"
#include "snmp_errors.h"

inherit "protocol";

private array get_community_list=({});
private array set_community_list=({});
private array managers_list=({});
private mapping oid_get_callbacks=([]);
private mapping oid_set_callbacks=([]);
private int managers_security_mode=0;


private void request_received(mapping rdata) {

  set_blocking();
  mapping rv=decode_asn1_msg(rdata);

  mapping varlist=([]);

  array n=indices(rv);
  
  if(rv[n[0]])
  {
      if(managers_security_mode && search(managers_list, 
        get_host_from_ip(rdata->ip))==-1) 
        // we want to check managers list, and this request is coming 
        // from a non manager, so let us ignore it.
      {
        return;
      }
    if(rv[n[0]]->op==Protocols.SNMP.REQUEST_GET || rv[n[0]]->op==Protocols.SNMP.REQUEST_GETNEXT)  // are we a Get?
    {
      if(search(get_community_list, rv[n[0]]->community)==-1) 
        // let us ignore bad communities
      {
        return;
      }
      foreach(rv[n[0]]->attribute, mapping att)
      {
         foreach(indices(att), string oid)
         {
           if(oid_get_callbacks[oid])
           {
             mixed r=oid_get_callbacks[oid](oid, rv[n[0]]);
             if(r[0]==0) // we had an error
             {
               return_error(rv, @r[1..]);
               return;
             }
             else varlist[oid]=r[1..];
           }
           else if(oid_get_callbacks["*"])
           {
             mixed r=oid_get_callbacks["*"](oid, rv[n[0]]);
             if(r[0]==0) // we had an error
             {
               return_error(rv, @r[1..]);
               return;
             }
             else varlist[oid]=r[1..];
            }
         }
      }
      if(varlist && sizeof(varlist)>0)
        get_response(varlist, rv);
    }
    else if(rv[n[0]]->op==Protocols.SNMP.REQUEST_SET)  // are we a Set?
    {
      if(search(set_community_list, rv[n[0]]->community)==-1) 
        // let us ignore bad communities
      {
        return;
      }
      foreach(rv[n[0]]->attribute, mapping att)
      {
         foreach(indices(att), string oid)
         {
           if(oid_set_callbacks[oid])
           {
             mixed r=oid_set_callbacks[oid](oid, att[oid], rv[n[0]]);
             if(r[0]==0) // we had an error
             {
               return_error(rv, @r[1..]);
               return;
             }
             else; // no error, so we should not panic.
           }
           else if(oid_set_callbacks["*"])
           {
             mixed r=oid_set_callbacks["*"](oid, att[oid], rv[n[0]]);
             if(r[0]==0) // we had an error
             {
               return_error(rv, @r[1..]);
               return;
             }
             else; // no error, so we should not panic.
            }
           else // we aren't supposed to let a non-handled oid go without error
           {
              return_error(rv, Protocols.SNMP.ERROR_NOSUCHNAME,
                 get_index_number(oid, rv[n[0]]));
              return;
           }
         }
      }
      // we made it to the end without error, so let's send success.
      return_error(rv, Protocols.SNMP.ERROR_NOERROR, 0);
    }
    else // we are not a Get/Set request...
    {
      error("unsupported request type " + rv[n[0]]->op + "\n");
    }
  }
}

//! create a new instance of the agent
//! 
//! @param port
//!   the port number to listen for requests on
//! @param addr
//!   the address to bind to for listening
//!
//! @note
//!    only one agent may be bound to a port at one time
//!    the agent does not currently support SMUX or AGENTX or other
//!    agent multiplexing protocols.
void create(int|void port, string|void addr) {
  int p=port|SNMP_DEFAULT_PORT;

  if(addr)
    ::create(0, 0, p, addr);
  else
    ::create(0, 0, p);

  ::set_read_callback(request_received);
  ::set_nonblocking();

}

//! enable manager access limits
//!
//! @param yesno
//! 1 to allow only managers to submit requests
//! 0 to allow any host to submit requests
//!
//! default setting allows all requests from all hosts
void set_managers_only(int yesno)
{
  managers_security_mode=yesno;
}

//! set the valid community strings for Get requests
//!
//! @param communities
//!    an array of valid Get communities
//!
//! @note
//!   send an empty array to disable Get requests
void set_get_communities(array communities)
{
   get_community_list=communities;
}

//! set the valid community strings for Set requests
//!
//! @param communities
//!    an array of valid Set communities
//!
//! @note
//!   send an empty array to disable Set requests
void set_set_communities(array communities)
{
   set_community_list=communities;
}

//! set the valid manager list for requests
//!
//! @param managers
//!    an array of valid management hosts
//!
//! @note
//!   send an empty array to disable all requests
void set_managers(array managers)
{
   managers_list=managers;
}

//! set the Set request callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to serve as the handler for all requests
//!   for which a handler is specified.
//! @param cb
//!   the function to call when oid is requested by a manager
//!   the function should take 3 arguments: a string containing the 
//!   requested oid, the desired value, and the body of the request as a mapping.
//!   The function should return an array containing 3 elements:
//!   The first is a boolean indicating success or failure.
//!   If success, the next 2 elements should be the SNMP data type of 
//!   the result followed by the result itself.
//!   If failure, the next 2 elements should be the error-status
//!   such as @[Protocols.SNMP.ERROR_TOOBIG] and the second
//!   is the error-index, if any.  
//! @note
//!    there can be only one callback per object identifier.
//!    calling this function more than once with the same oid will
//!    result in the new callback replacing the existing one.
void set_set_oid_callback(string oid, function cb)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("set_set_oid_callback(): invalid or no oid specified.\n");

  oid_set_callbacks[oid]=cb;    

}

//! clear the Set callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to indicate the default handler.
//!
//! @returns
//!    1 if the callback existed, 0 otherwise
int clear_set_oid_callback(string oid)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("clear_set_oid_callback(): invalid or no oid specified.\n");
  if(oid_set_callbacks[oid])
  {
    m_delete(oid_set_callbacks, oid);
    return 1;
  }
  else return 0;
}

//! get the Set request callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to indicate the default handler
//!
//! @returns
//!    the function associated with oid, if any
void|function get_set_oid_callback(string oid)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("get_set_oid_callback(): invalid or no oid specified.\n");
  if(oid_set_callbacks[oid])
    return oid_set_callbacks[oid];
}


//! set the Get request callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to serve as the handler for all requests
//!   for which a handler is specified.
//! @param cb
//!   the function to call when oid is requested by a manager
//!   the function should take 2 arguments: a string containing the 
//!   requested oid and the body of the request as a mapping.
//!   The function should return an array containing 3 elements:
//!   The first is a boolean indicating success or failure.
//!   If success, the next 2 elements should be the SNMP data type of 
//!   the result followed by the result itself.
//!   If failure, the next 2 elements should be the error-status
//!   such as @[Protocols.SNMP.ERROR_TOOBIG] and the second
//!   is the error-index, if any.  
//! @note
//!    there can be only one callback per object identifier.
//!    calling this function more than once with the same oid will
//!    result in the new callback replacing the existing one.
void set_get_oid_callback(string oid, function cb)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("set_get_oid_callback(): invalid or no oid specified.\n");

  oid_get_callbacks[oid]=cb;    

}

//! clear the Get callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to indicate the default handler.
//!
//! @returns
//!    1 if the callback existed, 0 otherwise
int clear_get_oid_callback(string oid)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("clear_get_oid_callback(): invalid or no oid specified.\n");
  if(oid_get_callbacks[oid])
  {
    m_delete(oid_get_callbacks, oid);
    return 1;
  }
  else return 0;
}

//! get the Get request callback function for an Object Identifier
//! 
//! @param oid
//!   the string object identifier, such as 1.3.6.1.4.1.132.2
//!   or an asterisk (*) to indicate the default handler
//!
//! @returns
//!    the function associated with oid, if any
void|function get_get_oid_callback(string oid)
{
  if(oid=="*"); // we can let * pass.
  else if(!oid || !is_valid_oid(oid)) 
    error("get_get_oid_callback(): invalid or no oid specified.\n");
  if(oid_get_callbacks[oid])
    return oid_get_callbacks[oid];
}

private int is_valid_oid(string o)
{
  array oid=o/".";
  int i;
  foreach(oid, string c)
  {
    string x;
    sscanf(c, "%[0-9]", x);
    if(c!=x) return 0;
  }
  return 1;
}

private string get_host_from_ip(string ip)
{
  string host=System.gethostbyaddr(ip)[0];
  return host;
}

private void return_error(mapping rv, int errstatus, int errindex)
{
   get_response(([]), rv, errstatus, errindex);
   return;
}

int get_index_number(string o, mapping r)
{
  for(int i=0; i< sizeof(r->attribute); i++)
  {
    if(r->attribute[i][o])
      return i;
  }
  return -1;
}
