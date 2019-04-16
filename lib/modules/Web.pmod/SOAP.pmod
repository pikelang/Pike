/*
 * Limited SOAP wsdl parser
 */

//! This is a limited SOAP wsdl parser using Promises.
//!
//! @example
//! @code
//! Web.SOAP.Promise("https://foo.bar/Logon.svc?wsdl")
//!  .on_success(lambda(Web.SOAP.Client soap) {
//!   Web.SOAP.Arguments args, sh;
//!   args               = soap->get_arguments("Logon");
//!   sh                 = args->LogonRequestMsg;
//!   sh->Username       = "foo";
//!   sh->Password       = "bar";
//!   sh->WebServiceType = "Publisher";
//!
//!   soap->call(args)
//!    .on_success(lambda(mapping resp) {
//!     string token = resp->CredentialToken;
//!   });
//! });
//! @endcode

#pike __REAL_VERSION__
#pragma dynamic_dot

//#define SOAP_DEBUG 1

#ifdef SOAP_DEBUG
#define PD(X ...)            werror(X)
                             // PT() puts this in the backtrace
#define PT(X ...)            (lambda(object _this){(X);}(this))
#else
#undef SOAP_DEBUGMORE
#define PD(X ...)            0
#define PT(X ...)            (X)
#endif

#define DERROR(msg ...)      ({sprintf(msg),backtrace()})

private constant defaultheaders = ([
  "Content-Type"    : "text/xml; charset=utf-8",
  "Accept-Encoding" : "gzip"
]);

//! @returns
//! A @[Concurrent.Future] that eventually delivers a @[Client].
//!
//! @seealso
//!   @[Client()]
final Concurrent.Future Promise(string wsdlurl, void|mapping headers) {
  if (headers)
    headers = defaultheaders + headers;
  else
    headers = defaultheaders;
  return Protocols.HTTP.Promise.get_url(wsdlurl,
   Protocols.HTTP.Promise.Arguments(([
    "headers":headers,
   ]))).map(Client);
}

//!
class Arguments {
  private mapping values;

  inline protected void create(mapping v) {
    values = v;
  }

  inline protected  mixed `[](string key) {
    return values[key];
  }

  inline protected mixed `->(string key) {
    return values[key];
  }

  protected mixed `[]=(string key, mixed v) {
    mixed vp = values[key];
    if (!vp || !arrayp(vp))
      DERROR("Invalid key %O\n", key);
    values[key] += ({v});
    return v;
  }

  inline protected mixed `->=(string key, mixed v) {
    return (this[key] = v);
  }

  inline protected Iterator _get_iterator() {
    return get_iterator(values);
  }

  inline protected array(string) _indices() {
    return filter(indices(values),
     lambda(mixed value) { return !has_prefix(value, "/"); });
  }

  // Clone the Arguments so that they can be reused,
  // mostly relevant for get_arguments().
  // A regular clone() method does not work here due to the override
  // on the -> and [] operators.
  protected Arguments `~() {
    mapping newvalues = values + ([]);
    foreach (newvalues; string key; mixed value)
      if (!has_prefix(key, "/"))
        if (arrayp(value))
          newvalues[key] += ({});
        else if (objectp(value))
          newvalues[key] = ~value;
    return Arguments(newvalues);
  }

  protected string _sprintf(int type, void|mapping flags) {
    string res = UNDEFINED;
    switch (type) {
      case 'O':
        res = sprintf("%O", values);
        int indent;
        if (flags && (indent = flags->indent))
          res = replace(res, "\n", "\n" + " " * indent);
        break;
    }
    return res;
  }
}

//!
class Client {
  private constant nsprefix = "n";
  private string serviceurl;
  private Arguments methods;
  private mapping nss = ([]);
  private string nextmethod;

  protected void _destruct() {
  }

  //! @returns
  //!   A structure describing all available methods on the current wsdl.
  final Arguments get_methods() {
    return methods;
  }

  //! Primes the @[Client] to perform the @[call()] later.  Use the
  //! returned structure to pass parameters.  Only assigned parameters
  //! will be passed to the call.
  //!
  //! @returns
  //!   A structure describing all arguments for the specified @expr{method@}.
  //!
  //! @seealso
  //!   @[call()]
  final Arguments get_arguments(string method) {
    return ~methods[nextmethod = method]->input;
  }

  //! Calls the SOAP method you most recently addressed using
  //! @[get_arguments()].
  //!
  //! @returns
  //! A @[Concurrent.Future] that eventually delivers a result mapping.
  //!
  //! @seealso
  //!   @[get_arguments()]
  final Concurrent.Future call(Arguments args) {

    string args2xml(Arguments args) {
      String.Buffer res = String.Buffer();
      array(string) keys = args["/sequence"] || indices(args);
      foreach (keys;; string key) {
        mixed value = args[key];
        if (objectp(value)) {
          string xv = args2xml(value);
          if (sizeof(xv)) {
            string ns = value["/"];
            res->sprintf("<%s:%s>%s</%s:%s>",
              ns, key, xv, ns, key);
          }
        } else if(arrayp(value) && sizeof(value) > 1) {
          int i, n = sizeof(value);
          string fmt = value[0];
          for (i = 0; ++i < n;)
            res->sprintf(fmt, key, value[i], key);
        }
      }
      return res->get();
    }

    String.Buffer req = String.Buffer();
    req->sprintf("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
     "<SOAP-ENV:Envelope"
      " xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\""
      "%{ xmlns:%s=\"%s\"%}>"
     "<SOAP-ENV:Body>", (array)nss);
    req->add(args2xml(args));
    req->add("</SOAP-ENV:Body></SOAP-ENV:Envelope>");
    PD((string)req);
    mapping h = defaultheaders + ([]);
    string body = string_to_utf8(req->get());
    h->SOAPAction = methods[nextmethod]["/action"];
    PD("SOAPAction: %O\n", h->SOAPAction);
    h["Content-Length"] = (string) sizeof(body);

    return Protocols.HTTP.Promise.post_url(serviceurl,
     Protocols.HTTP.Promise.Arguments(([
      "headers":h,
      "data":body,
     ]))).map(lambda(Protocols.HTTP.Promise.Result resp) {
      string res = resp->get();
      PD("\nGot: %O\n", res);
      Parser.XML.NSTree.NSNode root = Parser.XML.NSTree.parse_input(res);
      mapping m = Parser.XML.node_to_struct(root)->Envelope->Body;
      root->zap_tree();
      PD("\nGotmapping: %O\n", m);
      return m;
    });
  }

  //! Usually not called directly.
  //!
  //! @seealso
  //!   @[Promise()]
  protected void create(Protocols.HTTP.Promise.Result resp) {
    if (resp->content_type != "text/xml")
      DERROR("Invalid wsdl response %O: %O\n", resp->content_type, resp->get());
    Parser.XML.NSTree.NSNode root = Parser.XML.NSTree.parse_input(resp->get());
    mapping m = Parser.XML.node_to_struct(root)->definitions;
    root->zap_tree();
    PD("wsdl: %O\n", m);
    mapping types = ([]);
    serviceurl = m->service->port->address["/"]->location;
    {
      int nsi = 0;

      void proctypes(mapping types, array schema) {
        foreach (schema;; mapping fm) {
          string targetns = fm["/"]->targetNamespace;
          mapping collect;
          {
            string ns = nsprefix + (targetns ? nsi++ : nsi-1);
            collect = (["/":ns]);
            if (targetns) {
              types[targetns] = collect;
              nss[ns] = targetns;
            } else
              collect = types;
          }
          mapping|array elms;
          if (elms = fm->element) {
            if (mappingp(elms))
              elms = ({elms});
            foreach (elms;; mapping dtype) {
              string|mapping typ = dtype["/"]->type;
              if (typ)
                collect[dtype["/"]->name] = typ;
              else {
                dtype->complexType["/"] = (["name": dtype["/"]->name]);
                proctypes(collect, ({dtype}));
              }
            }
          }
          if (elms = fm->simpleType) {
            if (mappingp(elms))
              elms = ({elms});
            foreach (elms;; mapping dtype) {
              collect[dtype["/"]->name] = dtype->restriction["/"]->base;
            }
          }
          if (elms = fm->complexType) {
            if (targetns)
              collect["/type"] = "complex";
            if (mappingp(elms))
              elms = ({elms});
            foreach (elms;; mapping dtype) {
              mapping|array ctypes = dtype->sequence->element;
              if (mappingp(ctypes))
                ctypes = ({ctypes});
              mapping members = ([]);
              array sortseq = ({});
              foreach (ctypes;; mapping member) {
                string key = member["/"]->name;
                members[key] = member["/"]->type;
                sortseq += ({key});
              }
              if (sizeof(sortseq) > 1)
                members["/sequence"] = sortseq;
              collect[dtype["/"]->name] = members;
            }
          }
        }
      }

      proctypes(types, m->types->schema);
    }
    mapping tmethods = ([]);
    PD("Types: %O\n", types);
    {
      mapping msgs = ([]);
      foreach (m->message;; mapping fm)
        msgs[fm["/"]->name] = fm->part["/"]->element;
      foreach (m->binding->operation;; mapping fm) {
        string name = fm["/"]->name;
        mapping res = (["/action": fm->operation["/"]->soapAction]);
        fm -= (<"/", "operation">);
        foreach (fm; string type; mapping d) {
          string s = d["/"]->name;
          res[type] = msgs[s] || s;
        }
        tmethods[name] = res;
      }
    }

    mapping(string:multiset) visited = ([]);
    array|mapping repltypes(array|mapping f) {

      array sprintftype(string key, mixed type) {
        string rkey;
        sscanf(key, "%s:%s", key, rkey);
        type = repltypes(type);
        if (rkey) {
          if (arrayp(type))
            type = "string";
          if (stringp(type)) {
            switch (type) {
              case "dateTime":
              case "string":
              case "boolean":
                type = "%s";
                break;
              case "int":
                type = "%d";
                break;
              case "decimal":
              case "double":
                type = "%f";
                break;
              default:
                type = "%{"+type+"}";
                break;
            }
            type = ({sprintf("<%s:%%s>%s</%s:%%s>", key, type, key)});
            key = rkey;
          } else if (mappingp(type)) {
            type["/"] = key;
            key = rkey;
          }
        }
        return ({key, type});
      }

      if (arrayp(f) && sizeof(f) == 2
       && stringp(f[0]) && stringp(f[1])) {
        mapping sub = types[f[0]];
        mixed var = f[1];
        mixed res;
        if (sub) {
          if (res = sub[var]) {
            multiset vis = visited[f[0]];
            if (!vis)
              visited[f[0]] = vis = (<>);
            if (vis[var])
              f = var;
            else {
              vis[var] = 1;
              string ns = sub["/"]+":";
              if (mappingp(res)) {
                mapping nm = ([]);
                foreach (res; string key; mixed value)
                  nm[has_prefix(key, "/") ? key : ns+key] = value;
                res = nm;
              }
              string typ = sub["/type"];
              res = sprintftype(ns+var, res);
              vis[var] = 0;
              f = typ && typ == "complex" ? res[1] : ([res[0]:res[1]]);
            }
          }
        } else
          f = var;
      } else
        foreach (f; string key; mixed res)
          if (mappingp(res) || arrayp(res) && !has_prefix(key, "/")) {
            res = sprintftype(key, res);
            if (mappingp(f))
              m_delete(f, key);
            f[res[0]] = res[1];
          }
      return f;
    }

    tmethods = repltypes(tmethods);
    PD("Methods: %O\n", tmethods);
    {
      multiset usednss = (<>);
      void collectnss(mapping f) {
        foreach (f; string k; mixed v) {
          string ns;
          if (mappingp(v)) {
            collectnss(v);
            ns = v["/"];
            f[k] = Arguments(v);
          } else if (arrayp(v))
            sscanf(v[0], "<%s:", ns);
          if (ns)
            usednss[ns] = 1;
        }
      }

      collectnss(tmethods);
      methods = Arguments(tmethods);
      nss &= usednss;
    }
  }
}
