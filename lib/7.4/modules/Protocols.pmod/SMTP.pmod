#pike 7.5

class protocol {
  inherit Protocols.SMTP.Protocol;
}

class client {
  inherit Protocols.SMTP.Client;
  mapping(int:string) reply_codes = Protocols.SMTP.replycodes;
}
