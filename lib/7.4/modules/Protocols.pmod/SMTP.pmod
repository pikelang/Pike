#pike 7.5

//! @deprecated Protocol
class protocol {
  inherit Protocols.SMTP.Protocol;
}

//! @deprecated Client
class client {
  inherit Protocols.SMTP.Client;
  mapping(int:string) reply_codes = Protocols.SMTP.replycodes;
}
